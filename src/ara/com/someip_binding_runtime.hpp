#pragma once

#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <vsomeip/vsomeip.hpp>

#include "ara/com/runtime.h"
#include "com_runtime_state.hpp"

namespace ara::com::runtime::internal {

class SomeIpBindingRuntime final : public IBindingRuntime {
public:
    SomeIpBindingRuntime();
    ~SomeIpBindingRuntime() override;

    ara::core::Result<void> OfferService(const ServiceRecord& record) noexcept override;
    
    ara::core::Result<void>
    StopOfferService(const ara::com::ServiceIdentifierType& service_id,
                     const ara::com::InstanceIdentifier& instance_identifier) noexcept override;
                     
    ara::core::Result<ara::core::Vector<BindingMetadata>>
    FindServices(const ara::com::ServiceIdentifierType& service_id,
                 const ara::com::InstanceIdentifier& instance_identifier) noexcept override;

    ara::core::Result<void> PublishEvent(
        std::string_view channel_name, std::string_view payload) noexcept override;

    ara::core::Result<std::optional<ara::core::String>> GetNewEvent(
        std::string_view channel_name, std::uint64_t& last_seen_sequence) noexcept override;

    ara::core::Result<ara::core::String> CallMethod(
        std::string_view channel_name, std::string_view payload,
        std::chrono::milliseconds timeout) noexcept override;

    ara::core::Result<std::optional<ChannelMessage>> TakeMethodCall(
        std::string_view channel_name, std::uint64_t& last_seen_sequence) noexcept override;

    ara::core::Result<void> SendMethodResponse(
        std::string_view channel_name, std::uint64_t correlation_id,
        std::string_view payload) noexcept override;

    ara::core::Result<void> SendFireAndForget(
        std::string_view channel_name, std::string_view payload) noexcept override;

    ara::core::Result<std::optional<ara::core::String>> TakeFireAndForget(
        std::string_view channel_name, std::uint64_t& last_seen_sequence) noexcept override;

private:
    struct ChannelIds {
        vsomeip::service_t service_id;
        vsomeip::instance_t instance_id;
        vsomeip::method_t method_id; // or event_id
        vsomeip::eventgroup_t eventgroup_id; // Used for events
    };

    std::optional<ChannelIds> ParseChannelName(std::string_view channel_name) const;

    void StartVsomeipThread();
    void StopVsomeipThread();

    std::shared_ptr<vsomeip::application> app_;
    std::thread app_thread_;
    std::atomic<bool> app_running_{false};

    // State to buffer received events and method requests
    std::mutex state_mutex_;
    
    struct EventBuffer {
        std::uint64_t latest_sequence{0};
        ara::core::String payload;
    };
    std::map<std::string, EventBuffer> event_buffers_;

    struct RequestBuffer {
        std::uint64_t sequence{0};
        ChannelMessage message;
    };
    std::map<std::string, std::vector<RequestBuffer>> request_buffers_;
    std::map<std::uint64_t, std::shared_ptr<vsomeip::message>> pending_requests_;
    
    struct ResponseBuffer {
        ara::core::String payload;
        bool ready{false};
    };
    std::map<vsomeip::request_t, ResponseBuffer> response_buffers_;
    std::uint64_t next_correlation_id_{1};

    void OnMessageReceived(const std::shared_ptr<vsomeip::message>& msg);
};

} // namespace ara::com::runtime::internal
