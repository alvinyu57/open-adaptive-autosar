#include <atomic>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <thread>

#include "ara/core/initialization.h"
#include "ara/exec/execution_client.h"
#include "ara/log/logger.hpp"
#include "tp_provider_common.h"
#include "tp_provider_skeleton.h"
#include "tp_service_manifest.h"

namespace {

std::string DescribeSample(const openaa::tire_pressure::TirePressureSample& sample) {
    std::ostringstream stream;
    for (std::size_t index = 0; index < sample.readings.size(); ++index) {
        const auto& reading = sample.readings[index];
        stream << reading.position << '=' << reading.pressure_kpa << "kPa";
        if (index + 1U != sample.readings.size()) {
            stream << ", ";
        }
    }
    return stream.str();
}

// Get the path to the data directory relative to the executable
std::filesystem::path GetDataDirectory() {
    // Try to get the directory of the executable
    // First, try /proc/self/exe (Linux)
    try {
        std::filesystem::path exe_path = std::filesystem::read_symlink("/proc/self/exe");
        return exe_path.parent_path() / "data";
    } catch (const std::exception&) {
        // Fallback 1: If /proc/self/exe is not available, try relative path from CWD
        if (std::filesystem::exists("data")) {
            return std::filesystem::path("data");
        }
        // Fallback 2: Try relative path from build output
        if (std::filesystem::exists("apps/02_tire_pressure/data")) {
            return std::filesystem::path("apps/02_tire_pressure/data");
        }
        // Fallback 3: Try ../data if running from parent directory
        if (std::filesystem::exists("../data")) {
            return std::filesystem::path("../data");
        }
        // Default fallback
        return std::filesystem::path("data");
    }
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
            logger.Warn("tire_provider", "Received SIGTERM");
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
            logger.Error("tire_provider", "Failed to load service_instance_manifest.json");
            return 1;
        }

        openaa::tire_pressure::TirePressureProviderSkeleton skeleton(manifest_result.Value());
        if (!skeleton.OfferService().HasValue()) {
            logger.Error("tire_provider", "Failed to offer tire pressure service");
            return 1;
        }

        auto managed_execution_client = std::move(execution_client.Value());
        (void)managed_execution_client;

        while (keep_running.load()) {
            // read sample from json in data/tire_pressure_data.json
            std::filesystem::path data_file = GetDataDirectory() / "tire_pressure_data.json";
            auto sample_result = openaa::tire_pressure::LoadSampleFromJsonFile(data_file);

            if (!sample_result.HasValue()) {
                logger.Error("tire_provider", "Failed to load tire pressure JSON data");
                return 1;
            }

            auto sample = sample_result.Value();
            logger.Info("tire_provider",
                        std::string("Loaded tire pressure data: ") + DescribeSample(sample));

            sample.timestamp = std::chrono::system_clock::now();
            if (!skeleton.Publish(sample).HasValue()) {
                logger.Error("tire_provider", "Failed to publish tire pressure sample");
                break;
            }

            if (!skeleton.ProcessNextMethodCall().HasValue()) {
                logger.Warn("tire_provider", "Failed to process request/response call");
            }

            auto one_way_result = skeleton.ProcessNextFireAndForget();
            if (one_way_result.HasValue()) {
                if (const auto& opt = one_way_result.Value(); opt.has_value()) {
                    logger.Warn("tire_provider",
                                std::string("Received fire-and-forget notification: ") + *opt);
                }
            }

            logger.Info("tire_provider",
                        "Published tire pressure sample over ara::com shared memory IPC:");
            logger.Info("tire_provider",
                        "{FL: " + std::to_string(sample.readings[0].pressure_kpa) +
                            "kPa, FR: " + std::to_string(sample.readings[1].pressure_kpa) +
                            "kPa, RL: " + std::to_string(sample.readings[2].pressure_kpa) +
                            "kPa, RR: " + std::to_string(sample.readings[3].pressure_kpa) +
                            "kPa }");
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
        }

        (void)skeleton.StopOfferService();
        return ara::core::Deinitialize().HasValue() ? 0 : 1;
    } catch (const ara::core::ErrorCode& error) {
        std::cerr << "Error: " << error.Message() << '\n';
        return EXIT_FAILURE;
    } catch (...) {
        std::cerr << "An unexpected error occurred.\n";
        return EXIT_FAILURE;
    }
}