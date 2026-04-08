#pragma once

#include <filesystem>
#include <fstream>

#include "ara/com/runtime.h"
#include "tp_provider_common.h"

namespace openaa::tire_pressure {

class TirePressureProviderSkeleton final {
public:
    TirePressureProviderSkeleton()
        : instance_identifier_(TirePressureInstanceIdentifier()),
          endpoint_(TirePressureSnapshotPath()) {}

    ara::core::Result<void> OfferService() noexcept {
        const auto instance_specifier = TirePressureInstanceSpecifier();
        auto mapping_result = ara::com::runtime::internal::RegisterInstanceMapping(
            instance_specifier,
            instance_identifier_,
            {ara::com::runtime::internal::BindingType::kIpc,
             ara::core::String(endpoint_.string())});
        if (!mapping_result.HasValue()) {
            return mapping_result;
        }

        return ara::com::runtime::internal::OfferService(
            TirePressureServiceIdentifier(),
            instance_identifier_,
            {ara::com::runtime::internal::BindingType::kIpc,
             ara::core::String(endpoint_.string())});
    }

    ara::core::Result<void> StopOfferService() noexcept {
        return ara::com::runtime::internal::StopOfferService(TirePressureServiceIdentifier(),
                                                             instance_identifier_);
    }

    ara::core::Result<void> Publish(const TirePressureSample& sample) noexcept {
        std::error_code error_code;
        std::filesystem::create_directories(endpoint_.parent_path(), error_code);
        if (error_code) {
            return ara::core::Result<void>{
                ara::core::MakeErrorCode(ara::core::CoreErrc::kInvalidState)};
        }

        const auto temp_file = endpoint_.parent_path() / "tire_pressure_snapshot.tmp";
        {
            std::ofstream output(temp_file, std::ios::trunc);
            if (!output.is_open()) {
                return ara::core::Result<void>{
                    ara::core::MakeErrorCode(ara::core::CoreErrc::kInvalidState)};
            }
            output << SerializeSample(sample);
        }

        std::filesystem::rename(temp_file, endpoint_, error_code);
        if (error_code) {
            return ara::core::Result<void>{
                ara::core::MakeErrorCode(ara::core::CoreErrc::kInvalidState)};
        }

        return ara::core::Result<void>{};
    }

private:
    ara::com::InstanceIdentifier instance_identifier_;
    std::filesystem::path endpoint_;
};

} // namespace openaa::tire_pressure
