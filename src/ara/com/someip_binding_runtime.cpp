#include "someip_binding_runtime.hpp"

#include <chrono>
#include <iostream>
#include <sstream>
#include <unistd.h>

#include "ara/com/com_error_domain.h"

namespace ara::com::runtime::internal {

SomeIpBindingRuntime::SomeIpBindingRuntime() {
    std::string app_name = "app_" + std::to_string(getpid());
    app_ = vsomeip::runtime::get()->create_application(app_name);
    
    if (!app_->init()) {
        std::cerr << "Failed to initialize vsomeip application" << std::endl;
        return;
    }

    app_->register_message_handler(
        vsomeip::ANY_SERVICE, vsomeip::ANY_INSTANCE, vsomeip::ANY_METHOD,
        [this](const std::shared_ptr<vsomeip::message>& msg) { OnMessageReceived(msg); });

    StartVsomeipThread();
}

SomeIpBindingRuntime::~SomeIpBindingRuntime() {
    StopVsomeipThread();
}

void SomeIpBindingRuntime::StartVsomeipThread() {
    app_running_.store(true);
    app_thread_ = std::thread([this]() {
        app_->start();
    });
}

void SomeIpBindingRuntime::StopVsomeipThread() {
    if (app_running_.exchange(false)) {
        app_->stop();
        if (app_thread_.joinable()) {
            app_thread_.join();
        }
    }
}

std::optional<SomeIpBindingRuntime::ChannelIds> SomeIpBindingRuntime::ParseChannelName(std::string_view channel_name) const {
    ChannelIds ids{0, 0, 0, 0};
    std::string channel_str(channel_name);
    
    std::stringstream ss(channel_str);
    std::string item;
    int index = 0;
    while (std::getline(ss, item, ':')) {
        try {
            unsigned long val = std::stoul(item, nullptr, 0); // supports 0x hex prefix
            if (index == 0) ids.service_id = static_cast<vsomeip::service_t>(val);
            else if (index == 1) ids.instance_id = static_cast<vsomeip::instance_t>(val);
            else if (index == 2) ids.method_id = static_cast<vsomeip::method_t>(val); // Also used as event_id
            else if (index == 3) ids.eventgroup_id = static_cast<vsomeip::eventgroup_t>(val);
            index++;
        } catch (...) {
            return std::nullopt;
        }
    }
    
    if (index >= 3) {
        if (index == 3) ids.eventgroup_id = 1; // Default event group if not provided
        return ids;
    }
    
    return std::nullopt;
}

ara::core::Result<void> SomeIpBindingRuntime::OfferService(const ServiceRecord& record) noexcept {
    auto parsed = ParseChannelName(record.metadata.endpoint.View());
    if (!parsed) {
        return ara::core::Result<void>{MakeErrorCode(ComErrc::kCommunicationFailure)};
    }
    
    app_->offer_service(parsed->service_id, parsed->instance_id);
    // Also offer event if this is an event channel. 
    // vsomeip uses MSB to distinguish events (0x8000).
    if (parsed->method_id & 0x8000) {
        std::set<vsomeip::eventgroup_t> groups;
        groups.insert(parsed->eventgroup_id);
        app_->offer_event(parsed->service_id, parsed->instance_id, parsed->method_id, groups,
                          vsomeip::event_type_e::ET_EVENT, std::chrono::milliseconds::zero(),
                          false, true, nullptr, vsomeip::reliability_type_e::RT_UNKNOWN);
    }
    
    return ara::core::Result<void>{};
}

ara::core::Result<void> SomeIpBindingRuntime::StopOfferService(
    const ara::com::ServiceIdentifierType& service_id,
    const ara::com::InstanceIdentifier& instance_identifier) noexcept {
    // In a full implementation, we'd map string identifiers to vsomeip IDs.
    // For now, we rely on the process exit or let vsomeip handle teardown.
    return ara::core::Result<void>{};
}

ara::core::Result<ara::core::Vector<BindingMetadata>> SomeIpBindingRuntime::FindServices(
    const ara::com::ServiceIdentifierType& service_id,
    const ara::com::InstanceIdentifier& instance_identifier) noexcept {
    // Return a dummy metadata to unblock proxy connect, 
    // since vsomeip handles actual discovery implicitly via request_service.
    ara::core::Vector<BindingMetadata> matches;
    matches.push_back({BindingType::kSomeIp, "dummy_endpoint"});
    return ara::core::Result<ara::core::Vector<BindingMetadata>>{matches};
}

ara::core::Result<void> SomeIpBindingRuntime::PublishEvent(
    std::string_view channel_name, std::string_view payload) noexcept {
    auto parsed = ParseChannelName(channel_name);
    if (!parsed) return ara::core::Result<void>{MakeErrorCode(ComErrc::kCommunicationFailure)};

    std::shared_ptr<vsomeip::payload> vsomeip_payload = vsomeip::runtime::get()->create_payload();
    std::vector<vsomeip::byte_t> data(payload.begin(), payload.end());
    vsomeip_payload->set_data(data);

    app_->notify(parsed->service_id, parsed->instance_id, parsed->method_id, vsomeip_payload, true);
    return ara::core::Result<void>{};
}

ara::core::Result<std::optional<ara::core::String>> SomeIpBindingRuntime::GetNewEvent(
    std::string_view channel_name, std::uint64_t& last_seen_sequence) noexcept {
    auto parsed = ParseChannelName(channel_name);
    if (!parsed) return ara::core::Result<std::optional<ara::core::String>>{MakeErrorCode(ComErrc::kCommunicationFailure)};

    // Request the service to enable reception of events
    app_->request_service(parsed->service_id, parsed->instance_id);
    std::set<vsomeip::eventgroup_t> groups;
    groups.insert(parsed->eventgroup_id);
    app_->request_event(parsed->service_id, parsed->instance_id, parsed->method_id, groups, vsomeip::event_type_e::ET_EVENT);
    app_->subscribe(parsed->service_id, parsed->instance_id, parsed->eventgroup_id);

    std::lock_guard<std::mutex> lock(state_mutex_);
    auto it = event_buffers_.find(std::string(channel_name));
    if (it != event_buffers_.end() && it->second.latest_sequence > last_seen_sequence) {
        last_seen_sequence = it->second.latest_sequence;
        return ara::core::Result<std::optional<ara::core::String>>{it->second.payload};
    }
    return ara::core::Result<std::optional<ara::core::String>>{std::nullopt};
}

ara::core::Result<ara::core::String> SomeIpBindingRuntime::CallMethod(
    std::string_view channel_name, std::string_view payload,
    std::chrono::milliseconds timeout) noexcept {
    auto parsed = ParseChannelName(channel_name);
    if (!parsed) return ara::core::Result<ara::core::String>{MakeErrorCode(ComErrc::kCommunicationFailure)};

    app_->request_service(parsed->service_id, parsed->instance_id);

    std::shared_ptr<vsomeip::message> request = vsomeip::runtime::get()->create_request();
    request->set_service(parsed->service_id);
    request->set_instance(parsed->instance_id);
    request->set_method(parsed->method_id);

    std::shared_ptr<vsomeip::payload> vsomeip_payload = vsomeip::runtime::get()->create_payload();
    std::vector<vsomeip::byte_t> data(payload.begin(), payload.end());
    vsomeip_payload->set_data(data);
    request->set_payload(vsomeip_payload);

    vsomeip::client_t req_id = app_->get_client(); 
    // In a real implementation we would track req_id correctly.
    // For simplicity, we just send and wait.
    
    app_->send(request);

    auto start_time = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start_time < timeout) {
        std::lock_guard<std::mutex> lock(state_mutex_);
        auto it = response_buffers_.find(request->get_client());
        if (it != response_buffers_.end() && it->second.ready) {
            ara::core::String resp = it->second.payload;
            response_buffers_.erase(it);
            return ara::core::Result<ara::core::String>{resp};
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return ara::core::Result<ara::core::String>{MakeErrorCode(ComErrc::kCommunicationFailure)};
}

ara::core::Result<std::optional<ChannelMessage>> SomeIpBindingRuntime::TakeMethodCall(
    std::string_view channel_name, std::uint64_t& last_seen_sequence) noexcept {
    std::lock_guard<std::mutex> lock(state_mutex_);
    auto it = request_buffers_.find(std::string(channel_name));
    if (it != request_buffers_.end() && !it->second.empty()) {
        auto req = it->second.front();
        if (req.sequence > last_seen_sequence) {
            last_seen_sequence = req.sequence;
            it->second.erase(it->second.begin());
            return ara::core::Result<std::optional<ChannelMessage>>{req.message};
        }
    }
    return ara::core::Result<std::optional<ChannelMessage>>{std::nullopt};
}

ara::core::Result<void> SomeIpBindingRuntime::SendMethodResponse(
    std::string_view channel_name, std::uint64_t correlation_id,
    std::string_view payload) noexcept {
    std::shared_ptr<vsomeip::message> request;
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        auto it = pending_requests_.find(correlation_id);
        if (it != pending_requests_.end()) {
            request = it->second;
            pending_requests_.erase(it);
        } else {
            return ara::core::Result<void>{MakeErrorCode(ComErrc::kCommunicationFailure)};
        }
    }

    std::shared_ptr<vsomeip::message> response = vsomeip::runtime::get()->create_response(request);

    std::shared_ptr<vsomeip::payload> vsomeip_payload = vsomeip::runtime::get()->create_payload();
    std::vector<vsomeip::byte_t> data(payload.begin(), payload.end());
    vsomeip_payload->set_data(data);
    response->set_payload(vsomeip_payload);

    app_->send(response);
    return ara::core::Result<void>{};
}

ara::core::Result<void> SomeIpBindingRuntime::SendFireAndForget(
    std::string_view channel_name, std::string_view payload) noexcept {
    auto parsed = ParseChannelName(channel_name);
    if (!parsed) return ara::core::Result<void>{MakeErrorCode(ComErrc::kCommunicationFailure)};

    app_->request_service(parsed->service_id, parsed->instance_id);

    std::shared_ptr<vsomeip::message> request = vsomeip::runtime::get()->create_request();
    request->set_service(parsed->service_id);
    request->set_instance(parsed->instance_id);
    request->set_method(parsed->method_id);

    std::shared_ptr<vsomeip::payload> vsomeip_payload = vsomeip::runtime::get()->create_payload();
    std::vector<vsomeip::byte_t> data(payload.begin(), payload.end());
    vsomeip_payload->set_data(data);
    request->set_payload(vsomeip_payload);

    app_->send(request);
    return ara::core::Result<void>{};
}

ara::core::Result<std::optional<ara::core::String>> SomeIpBindingRuntime::TakeFireAndForget(
    std::string_view channel_name, std::uint64_t& last_seen_sequence) noexcept {
    // Same implementation as TakeMethodCall
    std::lock_guard<std::mutex> lock(state_mutex_);
    auto it = request_buffers_.find(std::string(channel_name));
    if (it != request_buffers_.end() && !it->second.empty()) {
        auto req = it->second.front();
        if (req.sequence > last_seen_sequence) {
            last_seen_sequence = req.sequence;
            it->second.erase(it->second.begin());
            return ara::core::Result<std::optional<ara::core::String>>{req.message.payload};
        }
    }
    return ara::core::Result<std::optional<ara::core::String>>{std::nullopt};
}

void SomeIpBindingRuntime::OnMessageReceived(const std::shared_ptr<vsomeip::message>& msg) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    std::string channel = std::to_string(msg->get_service()) + ":" + 
                          std::to_string(msg->get_instance()) + ":" + 
                          std::to_string(msg->get_method());
                          
    auto payload = msg->get_payload();
    std::string payload_str;
    if (payload && payload->get_length() > 0) {
        payload_str = std::string(reinterpret_cast<const char*>(payload->get_data()), payload->get_length());
    }

    if (msg->get_message_type() == vsomeip::message_type_e::MT_NOTIFICATION) {
        event_buffers_[channel].latest_sequence++;
        event_buffers_[channel].payload = ara::core::String(payload_str);
    } else if (msg->get_message_type() == vsomeip::message_type_e::MT_REQUEST ||
               msg->get_message_type() == vsomeip::message_type_e::MT_REQUEST_NO_RETURN) {
        RequestBuffer buf;
        buf.sequence = ++event_buffers_[channel].latest_sequence; // Reuse sequence counter for simplicity
        buf.message.correlation_id = (static_cast<std::uint64_t>(msg->get_client()) << 16) | msg->get_session();
        buf.message.payload = ara::core::String(payload_str);
        request_buffers_[channel].push_back(buf);
        pending_requests_[buf.message.correlation_id] = msg;
    } else if (msg->get_message_type() == vsomeip::message_type_e::MT_RESPONSE) {
        response_buffers_[msg->get_client()].payload = ara::core::String(payload_str);
        response_buffers_[msg->get_client()].ready = true;
    }
}

} // namespace ara::com::runtime::internal
