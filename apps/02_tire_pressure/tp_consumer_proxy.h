#pragma once

#include <atomic>
#include <chrono>
#include <mutex>
#include <optional>
#include <thread>

#include "ara/com/runtime.h"
#include "ara/com/service/sample_ptr.h"
#include "ara/com/types.h"
#include "tp_provider_common.h"

namespace openaa::tire_pressure {

class TirePressureConsumerProxy final {
public:
    explicit TirePressureConsumerProxy(TirePressureServiceManifest manifest)
        : manifest_(std::move(manifest)) {}

    ~TirePressureConsumerProxy() noexcept {
        StopSubscription();
    }

    TirePressureConsumerProxy(const TirePressureConsumerProxy&) = delete;
    TirePressureConsumerProxy& operator=(const TirePressureConsumerProxy&) = delete;
    TirePressureConsumerProxy(TirePressureConsumerProxy&&) = default;
    TirePressureConsumerProxy& operator=(TirePressureConsumerProxy&&) = default;

    ara::core::Result<void> Connect() noexcept {
        auto resolve_result = ara::com::runtime::ResolveInstanceIDs(manifest_.instance_specifier);
        if (resolve_result.HasValue() && !resolve_result.Value().empty()) {
            instance_identifier_ = resolve_result.Value().front();
        } else {
            instance_identifier_ = manifest_.instance_identifier;
            return ara::core::Result<void>{};
        }

        auto find_result = ara::com::runtime::internal::FindServices(manifest_.service_identifier,
                                                                     *instance_identifier_);
        if (!find_result.HasValue() || find_result.Value().empty()) {
            instance_identifier_ = manifest_.instance_identifier;
            return ara::core::Result<void>{};
        }

        return ara::core::Result<void>{};
    }

    ara::core::Result<void> Subscribe(ara::com::TriggerReceiveHandler handler) noexcept {
        if (!instance_identifier_.has_value()) {
            auto connect_result = Connect();
            if (!connect_result.HasValue()) {
                return connect_result;
            }
        }

        StopSubscription();
        running_.store(true);
        receiver_thread_ = std::thread([this, handler = std::move(handler)]() mutable {
            while (running_.load()) {
                auto event_result = ara::com::runtime::internal::GetNewEvent(
                    manifest_.event_channel, last_event_sequence_);
                if (event_result.HasValue()) {
                    const auto& opt = event_result.Value();
                    if (opt.has_value()) {
                        auto sample = DeserializeSample(opt->View());
                        if (sample.has_value()) {
                            {
                                std::lock_guard<std::mutex> lock(mutex_);
                                cached_sample_ = std::move(sample);
                            }
                            handler();
                        }
                    }
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(100));
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

        try {
            return ara::com::SamplePtr<TirePressureSample>(
                new TirePressureSample(*cached_sample_));
        } catch (const std::bad_alloc&) {
            return ara::com::SamplePtr<TirePressureSample>(nullptr);
        }
    }

    ara::core::Result<TirePressureSample> GetLatestByRequest() {
        auto response_result =
            ara::com::runtime::internal::CallMethod(manifest_.method_channel,
                                                    SerializeMethodRequest("GetLatestPressure"),
                                                    std::chrono::milliseconds(500));
        if (!response_result.HasValue()) {
            return ara::core::Result<TirePressureSample>{response_result.Error()};
        }

        auto sample = DeserializeSample(response_result.Value().View());
        if (!sample.has_value()) {
            return ara::core::Result<TirePressureSample>{
                ara::core::MakeErrorCode(ara::core::CoreErrc::kInvalidState)};
        }

        return ara::core::Result<TirePressureSample>{std::move(*sample)};
    }

    ara::core::Result<void> SendLowPressureAlarm(std::string_view detail) noexcept {
        return ara::com::runtime::internal::SendFireAndForget(
            manifest_.fire_and_forget_channel,
            SerializeFireAndForgetMessage("LowPressureAlarm", detail));
    }

private:
    TirePressureServiceManifest manifest_;
    std::optional<ara::com::InstanceIdentifier> instance_identifier_;
    std::atomic<bool> running_{false};
    std::thread receiver_thread_;
    mutable std::mutex mutex_;
    std::optional<TirePressureSample> cached_sample_;
    std::uint64_t last_event_sequence_{0U};
};

} // namespace openaa::tire_pressure
