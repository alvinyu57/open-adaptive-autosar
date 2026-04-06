#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

#include "ara/core/initialization.h"
#include "ara/exec/execution_client.h"
#include "ara/log/logger.hpp"

int main(int argc, char* argv[]) {
    if (!ara::core::Initialize(argc, argv).HasValue()) {
        return 1;
    }

    ara::log::Logger logger(std::cout);
    std::atomic<bool> keep_running{true};
    auto execution_client = ara::exec::ExecutionClient::Create([&keep_running, &logger]() {
        logger.Warn("hello_world", "Received SIGTERM");
        keep_running.store(false);
    });
    if (!execution_client.HasValue()) {
        return 1;
    }

    if (!execution_client.Value()
             .ReportExecutionState(ara::exec::ExecutionState::kRunning)
             .HasValue()) {
        return 1;
    }

    logger.Info("hello_world",
                "Adaptive AUTOSAR hello-world process is running under Execution Management");

    auto managed_execution_client = std::move(execution_client.Value());
    (void)managed_execution_client;

    for (int i = 0; i < 5; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        logger.Info("hello_world", "Running ...");
    }

    return ara::core::Deinitialize().HasValue() ? 0 : 1;
}
