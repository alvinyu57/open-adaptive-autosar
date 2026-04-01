#include <iostream>

#include "ara/core/initialization.h"
#include "ara/core/instance_specifier.h"
#include "ara/exec/execution_client.h"
#include "ara/exec/function_group.h"
#include "ara/exec/function_group_state.h"
#include "ara/exec/state_client.h"
#include "ara/log/logger.hpp"

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

    if (!execution_client.Value().ReportExecutionState(ara::exec::ExecutionState::kRunning)
             .HasValue()) {
        std::cerr << "Failed to report execution state" << std::endl;
        return 1;
    }
    logger.Info("smoke", "Reported execution state: kRunning");

    auto state_client = ara::exec::StateClient::Create(
        [](const ara::exec::ExecutionErrorEvent&) {});
    if (!state_client.HasValue()) {
        return 1;
    }

    ara::exec::FunctionGroup machine_fg(ara::core::InstanceSpecifier("MachineFG"));
    auto transition =
        state_client.Value().SetState(ara::exec::FunctionGroupState(machine_fg, "Startup"));
    logger.Info("smoke", "Requested machine state transition to 'Startup'...");
    transition.get();
    logger.Info("smoke", "Transition to 'Startup' complete.");

    if (!ara::core::Deinitialize().HasValue()) {
        std::cerr << "Failed to deinitialize ara::core" << std::endl;
        return 1;
    }
    std::cout << "[SmokeTest] ara::core deinitialized." << std::endl;
    return 0;
}
