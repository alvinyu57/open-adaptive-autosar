#pragma once

#include <optional>

#include "ara/com/runtime.h"
#include "tp_provider_common.h"

namespace openaa::tire_pressure {

class TirePressureProviderSkeleton final {
public:
    explicit TirePressureProviderSkeleton(TirePressureServiceManifest manifest)
        : manifest_(std::move(manifest)) {}

    ara::core::Result<void> OfferService() noexcept {
        auto mapping_result = ara::com::runtime::internal::RegisterInstanceMapping(
            manifest_.instance_specifier,
            manifest_.instance_identifier,
            {ara::com::runtime::internal::BindingType::kIpc,
             ara::core::String(manifest_.event_channel)});
        if (!mapping_result.HasValue()) {
            return mapping_result;
        }

        return ara::com::runtime::internal::OfferService(
            manifest_.service_identifier,
            manifest_.instance_identifier,
            {ara::com::runtime::internal::BindingType::kIpc,
             ara::core::String(manifest_.event_channel)});
    }

    ara::core::Result<void> StopOfferService() noexcept {
        return ara::com::runtime::internal::StopOfferService(manifest_.service_identifier,
                                                             manifest_.instance_identifier);
    }

    ara::core::Result<void> Publish(const TirePressureSample& sample) noexcept {
        latest_sample_ = sample;
        return ara::com::runtime::internal::PublishEvent(ara::com::runtime::internal::BindingType::kIpc, manifest_.event_channel,
                                                         SerializeSample(sample));
    }

    ara::core::Result<void> ProcessNextMethodCall() {
        auto request_result = ara::com::runtime::internal::TakeMethodCall(manifest_.method_channel,
                                                                          last_method_sequence_);
        if (!request_result.HasValue()) {
            return ara::core::Result<void>{request_result.Error()};
        }
        if (!request_result.Value().has_value()) {
            return ara::core::Result<void>{};
        }

        if (!request_result.HasValue()) {
            return ara::core::Result<void>{request_result.Error()};
        }

        const auto& opt = request_result.Value();
        if (!opt.has_value()) {
            return ara::core::Result<void>{};
        }

        const auto& request = *opt;

        if (request.payload.View() == "GetLatestPressure") {
            const auto response = SerializeSample(latest_sample_.value_or(TirePressureSample{}));
            return ara::com::runtime::internal::SendMethodResponse(
                ara::com::runtime::internal::BindingType::kIpc, manifest_.method_channel, request.correlation_id, response);
        }

        return ara::com::runtime::internal::SendMethodResponse(
            ara::com::runtime::internal::BindingType::kIpc, manifest_.method_channel, request.correlation_id, "{}");
    }

    ara::core::Result<std::optional<std::string>> ProcessNextFireAndForget() {
        auto one_way_result = ara::com::runtime::internal::TakeFireAndForget(
            ara::com::runtime::internal::BindingType::kIpc, manifest_.fire_and_forget_channel, last_fire_and_forget_sequence_);
        if (!one_way_result.HasValue()) {
            return ara::core::Result<std::optional<std::string>>{one_way_result.Error()};
        }
        if (!one_way_result.Value().has_value()) {
            return ara::core::Result<std::optional<std::string>>{std::optional<std::string>{}};
        }

        if (!one_way_result.HasValue()) {
            return ara::core::Result<std::optional<std::string>>{std::optional<std::string>{}};
        }

        const auto& opt = one_way_result.Value();
        if (!opt) {
            return ara::core::Result<std::optional<std::string>>{std::optional<std::string>{}};
        }

        const auto parsed = DeserializeFireAndForgetMessage(opt->View());
        if (!parsed.has_value()) {
            return ara::core::Result<std::optional<std::string>>{std::optional<std::string>{}};
        }

        return ara::core::Result<std::optional<std::string>>{
            std::optional<std::string>{parsed->first + ":" + parsed->second}};
    }

private:
    TirePressureServiceManifest manifest_;
    std::optional<TirePressureSample> latest_sample_;
    std::uint64_t last_method_sequence_{0U};
    std::uint64_t last_fire_and_forget_sequence_{0U};
};

} // namespace openaa::tire_pressure
