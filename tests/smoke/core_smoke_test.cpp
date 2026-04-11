#include <chrono>
#include <iostream>
#include <optional>
#include <string>
#include <thread>
#include <unistd.h>

#include "ara/com/runtime.h"
#include "ara/core/initialization.h"
#include "ara/core/instance_specifier.h"
#include "ara/exec/execution_client.h"
#include "ara/exec/function_group.h"
#include "ara/exec/function_group_state.h"
#include "ara/exec/state_client.h"
#include "ara/log/logger.hpp"

namespace {

std::string UniqueName(const std::string& prefix) {
    return prefix + "_" + std::to_string(::getpid());
}

} // namespace

int main() {
    if (!ara::core::Initialize().HasValue()) {
        std::cerr << "Failed to initialize ara::core" << std::endl;
        return 1;
    }
    std::cout << "[SmokeTest] ara::core initialized." << std::endl;

    ara::log::Logger logger(std::cout);
    auto execution_client = ara::exec::ExecutionClient::Create([&logger]() {
        logger.Warn("smoke", "Received SIGTERM");
    });
    if (!execution_client.HasValue()) {
        return 1;
    }

    if (!execution_client.Value()
             .ReportExecutionState(ara::exec::ExecutionState::kRunning)
             .HasValue()) {
        std::cerr << "Failed to report execution state" << std::endl;
        return 1;
    }
    logger.Info("smoke", "Reported execution state: kRunning");

    auto state_client =
        ara::exec::StateClient::Create([](const ara::exec::ExecutionErrorEvent&) {});
    if (!state_client.HasValue()) {
        return 1;
    }

    ara::exec::FunctionGroup machine_fg(ara::core::InstanceSpecifier("MachineFG"));
    auto transition =
        state_client.Value().SetState(ara::exec::FunctionGroupState(machine_fg, "Startup"));
    logger.Info("smoke", "Requested machine state transition to 'Startup'...");
    transition.get();
    logger.Info("smoke", "Transition to 'Startup' complete.");

    const auto instance_specifier =
        ara::core::InstanceSpecifier::Create("/Smoke/AraCom/TirePressure").Value();
    const auto instance_identifier =
        ara::com::InstanceIdentifier::Create("smoke/tire-pressure/service");
    const ara::com::ServiceIdentifierType service_id("smoke.ara.com.TirePressureService");
    const auto endpoint = UniqueName("smoke_endpoint");
    const auto event_channel = UniqueName("smoke_event");
    const auto method_channel = UniqueName("smoke_method");
    const auto one_way_channel = UniqueName("smoke_oneway");

    auto register_result = ara::com::runtime::internal::RegisterInstanceMapping(
        instance_specifier,
        instance_identifier,
        {ara::com::runtime::internal::BindingType::kIpc, ara::core::String(endpoint)});
    if (!register_result.HasValue()) {
        std::cerr << "Failed to register ara::com instance mapping" << std::endl;
        return 1;
    }

    auto resolve_result = ara::com::runtime::ResolveInstanceIDs(instance_specifier);
    if (!resolve_result.HasValue() || resolve_result.Value().empty()) {
        std::cerr << "Failed to resolve ara::com instance ids" << std::endl;
        return 1;
    }

    auto offer_result = ara::com::runtime::internal::OfferService(
        service_id,
        instance_identifier,
        {ara::com::runtime::internal::BindingType::kIpc, ara::core::String(endpoint)});
    if (!offer_result.HasValue()) {
        std::cerr << "Failed to offer ara::com service" << std::endl;
        return 1;
    }

    auto find_result = ara::com::runtime::internal::FindServices(service_id, instance_identifier);
    if (!find_result.HasValue() || find_result.Value().empty()) {
        std::cerr << "Failed to find ara::com service" << std::endl;
        return 1;
    }

    logger.Info("smoke", "Resolved and discovered ara::com shared-memory service.");

    std::uint64_t last_event_sequence{0U};
    auto publish_result =
        ara::com::runtime::internal::PublishEvent(event_channel, R"({"pressureKPa":96})");
    if (!publish_result.HasValue()) {
        std::cerr << "Failed to publish ara::com event" << std::endl;
        return 1;
    }

    auto event_result =
        ara::com::runtime::internal::GetNewEvent(event_channel, last_event_sequence);
    if (!event_result.HasValue() || !event_result.Value().has_value()) {
        std::cerr << "Failed to receive ara::com event" << std::endl;
        return 1;
    }
    logger.Info("smoke", "Received ara::com event over shared memory.");

    std::uint64_t last_method_sequence{0U};
    std::optional<ara::core::Result<ara::core::String>> method_response;
    std::thread method_caller([&]() {
        method_response = ara::com::runtime::internal::CallMethod(
            method_channel,
            "GetLatestPressure",
            std::chrono::milliseconds(1000));
    });

    std::optional<ara::com::runtime::internal::ChannelMessage> method_request;
    for (int attempt = 0; attempt < 20 && !method_request.has_value(); ++attempt) {
        auto take_result =
            ara::com::runtime::internal::TakeMethodCall(method_channel, last_method_sequence);
        if (!take_result.HasValue()) {
            std::cerr << "Failed to take ara::com method call" << std::endl;
            method_caller.join();
            return 1;
        }
        method_request = take_result.Value();
        if (!method_request.has_value()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    }

    if (!method_request.has_value()) {
        std::cerr << "Timed out waiting for ara::com method request" << std::endl;
        method_caller.join();
        return 1;
    }

    auto send_response_result = ara::com::runtime::internal::SendMethodResponse(
        method_channel,
        method_request->correlation_id,
        R"({"pressureKPa":96})");
    if (!send_response_result.HasValue()) {
        std::cerr << "Failed to send ara::com method response" << std::endl;
        method_caller.join();
        return 1;
    }

    method_caller.join();
    if (!method_response.has_value() || !method_response->HasValue()) {
        std::cerr << "Failed to complete ara::com request/response" << std::endl;
        return 1;
    }
    logger.Info("smoke", "Completed ara::com request/response over shared memory.");

    std::uint64_t last_one_way_sequence{0U};
    auto send_one_way_result = ara::com::runtime::internal::SendFireAndForget(
        one_way_channel,
        "LowPressureAlarm:front_left=96");
    if (!send_one_way_result.HasValue()) {
        std::cerr << "Failed to send ara::com fire-and-forget message" << std::endl;
        return 1;
    }

    auto one_way_result =
        ara::com::runtime::internal::TakeFireAndForget(one_way_channel, last_one_way_sequence);
    if (!one_way_result.HasValue() || !one_way_result.Value().has_value()) {
        std::cerr << "Failed to receive ara::com fire-and-forget message" << std::endl;
        return 1;
    }
    logger.Info("smoke", "Completed ara::com fire-and-forget over shared memory.");

    (void)ara::com::runtime::internal::StopOfferService(service_id, instance_identifier);

    if (!ara::core::Deinitialize().HasValue()) {
        std::cerr << "Failed to deinitialize ara::core" << std::endl;
        return 1;
    }
    std::cout << "[SmokeTest] ara::core deinitialized." << std::endl;
    return 0;
}
