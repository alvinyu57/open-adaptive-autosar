#include <filesystem>
#include <iostream>

#include "ara/core/initialization.h"
#include "ara/core/instance_specifier.h"
#include "ara/exec/execution_client.h"
#include "ara/exec/function_group.h"
#include "ara/exec/function_group_state.h"
#include "ara/exec/state_client.h"
#include "ara/log/logger.hpp"

int main(int argc, char* argv[]) {
    if (!ara::core::Initialize(argc, argv).HasValue()) {
        return 1;
    }

    ara::log::Logger logger(std::cout);
    const std::filesystem::path manifest_path =
        argc > 1 ? std::filesystem::path(argv[1])
                 : std::filesystem::path(argv[0]).parent_path() / "manifests" /
                       "execution_manifest.json";

    auto execution_client = ara::exec::ExecutionClient::Create([&logger]() {
        logger.Warn("app_manifest", "Received SIGTERM");
    });
    if (!execution_client.HasValue()) {
        return 1;
    }

    if (!execution_client.Value().ReportExecutionState(ara::exec::ExecutionState::kRunning)
             .HasValue()) {
        return 1;
    }

    auto state_client = ara::exec::StateClient::Create([&logger](
                                                            const ara::exec::ExecutionErrorEvent&
                                                                event) {
        logger.Error(
            "app_manifest",
            "Undefined state callback for function group " +
                std::string(event.functionGroup.GetInstanceSpecifier().View()));
    });
    if (!state_client.HasValue()) {
        return 1;
    }

    ara::exec::FunctionGroup machine_fg(ara::core::InstanceSpecifier("MachineFG"));
    state_client.Value()
        .SetState(ara::exec::FunctionGroupState(machine_fg, "Startup"))
        .get();

    logger.Info(
        "app_manifest",
        "Execution manifest sample active at " + manifest_path.string());

    return ara::core::Deinitialize().HasValue() ? 0 : 1;
}
