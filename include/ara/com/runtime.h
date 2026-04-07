#pragma once

#include "ara/com/com_error_domain.h"
#include "ara/com/service/service_identifier.h"
#include "ara/com/service/service_version.h"
#include "ara/com/types.h"
#include "ara/core/instance_specifier.h"
#include "ara/core/result.h"

namespace ara::com::runtime {

ara::core::Result<ara::com::InstanceIdentifierContainer> ResolveInstanceIDs(
    ara::core::InstanceSpecifier meta_model_identifier) noexcept;

namespace internal {

enum class BindingType : std::uint8_t {
    kIpc = 0,
    kDds = 1,
};

struct BindingMetadata final {
    BindingType binding_type{BindingType::kIpc};
    ara::core::String endpoint;
};

ara::core::Result<void> RegisterInstanceMapping(
    const ara::core::InstanceSpecifier& instance_specifier,
    const ara::com::InstanceIdentifier& instance_identifier,
    BindingMetadata metadata = {}) noexcept;

ara::core::Result<void> OfferService(
    const ara::com::ServiceIdentifierType& service_id,
    const ara::com::InstanceIdentifier& instance_identifier,
    BindingMetadata metadata = {}) noexcept;

ara::core::Result<void> StopOfferService(
    const ara::com::ServiceIdentifierType& service_id,
    const ara::com::InstanceIdentifier& instance_identifier) noexcept;

ara::core::Result<ara::core::Vector<BindingMetadata>> FindServices(
    const ara::com::ServiceIdentifierType& service_id,
    const ara::com::InstanceIdentifier& instance_identifier) noexcept;

} // namespace internal

} // namespace ara::com::runtime
