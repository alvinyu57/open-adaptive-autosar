#include <atomic>
#include <chrono>
#include <iostream>
#include <sstream>
#include <thread>

#include "ara/core/initialization.h"
#include "ara/exec/execution_client.h"
#include "ara/log/logger.hpp"
#include "tp_consumer_common.h"
#include "tp_consumer_proxy.h"
#include "tp_service_manifest.h"

namespace {

std::string DescribeAlert(const openaa::tire_pressure::TirePressureSample& sample) {
    std::ostringstream stream;
    bool first = true;
    for (const auto& reading : sample.readings) {
        if (reading.pressure_kpa >= 100) {
            continue;
        }

        if (!first) {
            stream << ", ";
        }
        first = false;
        stream << reading.position << '=' << reading.pressure_kpa << "kPa";
    }

    return stream.str();
}

} // namespace

int main(int argc, char* argv[]) {
    try {
        if (!ara::core::Initialize(argc, argv).HasValue()) {
            return 1;
        }

        ara::log::Logger logger(std::cout);
        std::atomic<bool> keep_running{true};
        auto execution_client = ara::exec::ExecutionClient::Create([&keep_running, &logger]() {
            logger.Warn("tire_consumer", "Received SIGTERM");
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

        auto manifest_result =
            openaa::tire_pressure::LoadServiceManifest(OPEN_AA_TIRE_PRESSURE_SERVICE_MANIFEST);
        if (!manifest_result.HasValue()) {
            logger.Error("tire_consumer", "Failed to load service_instance_manifest.arxml");
            return 1;
        }

        openaa::tire_pressure::TirePressureConsumerProxy proxy(manifest_result.Value());
        bool connected = false;
        for (int attempt = 0; attempt < 20 && keep_running.load(); ++attempt) {
            if (proxy.Connect().HasValue()) {
                connected = true;
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        if (!connected) {
            logger.Error("tire_consumer", "Failed to connect to tire pressure service");
            return 1;
        }

        auto subscribe_result = proxy.Subscribe([&logger, &proxy]() {
            auto sample = proxy.GetSample();
            if (!sample) {
                return;
            }

            auto request_result = proxy.GetLatestByRequest();
            if (!request_result.HasValue()) {
                logger.Warn("tire_consumer", "Request/response validation failed");
            }

            if (openaa::tire_pressure::HasLowTirePressure(*sample, 100)) {
                logger.Warn("tire_consumer",
                            std::string("LOW TIRE PRESSURE ALERT: ") + DescribeAlert(*sample));
                (void)proxy.SendLowPressureAlarm(DescribeAlert(*sample));
                return;
            }

            logger.Info("tire_consumer", "All tire pressures are within the safe range");
        });
        if (!subscribe_result.HasValue()) {
            logger.Error("tire_consumer", "Failed to subscribe to tire pressure updates");
            return 1;
        }

        auto managed_execution_client = std::move(execution_client.Value());
        (void)managed_execution_client;

        for (int seconds_waited = 0; keep_running.load() && seconds_waited < 60; ++seconds_waited) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        proxy.StopSubscription();
        return ara::core::Deinitialize().HasValue() ? 0 : 1;
    } catch (const ara::core::ErrorCode& error) {
        std::cerr << "Error: " << error.Message() << '\n';
        return EXIT_FAILURE;
    } catch (...) {
        std::cerr << "An unexpected error occurred.\n";
        return EXIT_FAILURE;
    }
}