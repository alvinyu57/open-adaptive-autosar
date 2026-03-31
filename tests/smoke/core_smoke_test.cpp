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
        return 1;
    }

    ara::log::Logger logger(std::cout);
    auto execution_client = ara::exec::ExecutionClient::Create([&logger]() {
        logger.Warn("smoke", "Received SIGTERM");
    });
    if (!execution_client.HasValue()) {
        return 1;
    }

    if (!execution_client.Value().ReportExecutionState(ara::exec::ExecutionState::kRunning)
             .HasValue()) {
        return 1;
    }

    auto state_client = ara::exec::StateClient::Create(
        [](const ara::exec::ExecutionErrorEvent&) {});
    if (!state_client.HasValue()) {
        return 1;
    }

    ara::exec::FunctionGroup machine_fg(ara::core::InstanceSpecifier("MachineFG"));
    auto transition =
        state_client.Value().SetState(ara::exec::FunctionGroupState(machine_fg, "Startup"));
    transition.get();

    if (!ara::core::Deinitialize().HasValue()) {
        return 1;
    }
    return 0;
}
