#pragma once

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <optional>
#include <sstream>
#include <thread>

#include "ara/com/runtime.h"
#include "ara/com/service/sample_ptr.h"
#include "ara/com/types.h"
#include "tp_provider_common.h"

namespace openaa::tire_pressure {

class TirePressureConsumerProxy final {
public:
    TirePressureConsumerProxy() = default;

    ~TirePressureConsumerProxy() {
        StopSubscription();
    }

    ara::core::Result<void> Connect() noexcept {
        auto resolve_result =
            ara::com::runtime::ResolveInstanceIDs(TirePressureInstanceSpecifier());
        if (!resolve_result.HasValue()) {
            return ara::core::Result<void>{resolve_result.Error()};
        }

        if (resolve_result.Value().empty()) {
            return ara::core::Result<void>{
                ara::com::MakeErrorCode(ara::com::ComErrc::kServiceNotAvailable)};
        }

        instance_identifier_ = resolve_result.Value().front();
        auto find_result = ara::com::runtime::internal::FindServices(
            TirePressureServiceIdentifier(), *instance_identifier_);
        if (!find_result.HasValue()) {
            return ara::core::Result<void>{find_result.Error()};
        }

        if (find_result.Value().empty()) {
            return ara::core::Result<void>{
                ara::com::MakeErrorCode(ara::com::ComErrc::kServiceNotAvailable)};
        }

        endpoint_ = find_result.Value().front().endpoint.View();
        return ara::core::Result<void>{};
    }

    ara::core::Result<void> Subscribe(ara::com::TriggerReceiveHandler handler) noexcept {
        if (!instance_identifier_.has_value() || endpoint_.empty()) {
            auto connect_result = Connect();
            if (!connect_result.HasValue()) {
                return connect_result;
            }
        }

        StopSubscription();
        running_.store(true);
        receiver_thread_ = std::thread([this, handler = std::move(handler)]() mutable {
            while (running_.load()) {
                std::error_code error_code;
                const auto exists = std::filesystem::exists(endpoint_, error_code);
                if (!error_code && exists) {
                    const auto modified_time =
                        std::filesystem::last_write_time(endpoint_, error_code);
                    if (!error_code && (!last_modified_time_.has_value() ||
                                        *last_modified_time_ != modified_time)) {
                        last_modified_time_ = modified_time;
                        if (LoadLatestSample()) {
                            handler();
                        }
                    }
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }
        });

        return ara::core::Result<void>{};
    }

    void StopSubscription() noexcept {
        running_.store(false);
        if (receiver_thread_.joinable()) {
            receiver_thread_.join();
        }
    }

    ara::com::SamplePtr<TirePressureSample> GetSample() const noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!cached_sample_.has_value()) {
            return ara::com::SamplePtr<TirePressureSample>(nullptr);
        }

        return ara::com::SamplePtr<TirePressureSample>(new TirePressureSample(*cached_sample_));
    }

private:
    bool LoadLatestSample() noexcept {
        std::ifstream input(endpoint_);
        if (!input.is_open()) {
            return false;
        }

        std::ostringstream buffer;
        buffer << input.rdbuf();
        auto sample = DeserializeSample(buffer.str());
        if (!sample.has_value()) {
            return false;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        cached_sample_ = std::move(sample);
        return true;
    }

    std::optional<ara::com::InstanceIdentifier> instance_identifier_;
    std::filesystem::path endpoint_;
    std::atomic<bool> running_{false};
    std::thread receiver_thread_;
    mutable std::mutex mutex_;
    std::optional<TirePressureSample> cached_sample_;
    std::optional<std::filesystem::file_time_type> last_modified_time_;
};

} // namespace openaa::tire_pressure
