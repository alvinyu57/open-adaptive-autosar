#include "ara/com/runtime.h"

#include "com_runtime_state.hpp"

namespace ara::com::runtime {

ara::core::Result<ara::com::InstanceIdentifierContainer>
ResolveInstanceIDs(ara::core::InstanceSpecifier meta_model_identifier) noexcept {
    return internal::ComRuntimeState::Instance().ResolveInstanceIDs(meta_model_identifier);
}

namespace internal {

ara::core::Result<void>
RegisterInstanceMapping(const ara::core::InstanceSpecifier& instance_specifier,
                        const ara::com::InstanceIdentifier& instance_identifier,
                        BindingMetadata metadata) noexcept {
    return ComRuntimeState::Instance().RegisterInstanceMapping(
        instance_specifier, instance_identifier, metadata);
}

ara::core::Result<void> OfferService(const ara::com::ServiceIdentifierType& service_id,
                                     const ara::com::InstanceIdentifier& instance_identifier,
                                     BindingMetadata metadata) noexcept {
    return ComRuntimeState::Instance().OfferService(service_id, instance_identifier, metadata);
}

ara::core::Result<void>
StopOfferService(const ara::com::ServiceIdentifierType& service_id,
                 const ara::com::InstanceIdentifier& instance_identifier) noexcept {
    return ComRuntimeState::Instance().StopOfferService(service_id, instance_identifier);
}

ara::core::Result<ara::core::Vector<BindingMetadata>>
FindServices(const ara::com::ServiceIdentifierType& service_id,
             const ara::com::InstanceIdentifier& instance_identifier) noexcept {
    return ComRuntimeState::Instance().FindServices(service_id, instance_identifier);
}

} // namespace internal

} // namespace ara::com::runtime
