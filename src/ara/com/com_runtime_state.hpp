#pragma once

#include <map>
#include <mutex>
#include <string>

#include "ara/com/runtime.h"

namespace ara::com::runtime::internal {

struct ServiceRecord final {
    ara::com::ServiceIdentifierType service_id;
    ara::com::InstanceIdentifier instance_identifier;
    BindingMetadata metadata;
};

class IBindingRuntime {
public:
    virtual ~IBindingRuntime() = default;

    virtual ara::core::Result<void> OfferService(const ServiceRecord& record) noexcept = 0;
    virtual ara::core::Result<void> StopOfferService(
        const ara::com::ServiceIdentifierType& service_id,
        const ara::com::InstanceIdentifier& instance_identifier) noexcept = 0;
    virtual ara::core::Result<ara::core::Vector<BindingMetadata>> FindServices(
        const ara::com::ServiceIdentifierType& service_id,
        const ara::com::InstanceIdentifier& instance_identifier) noexcept = 0;
};

class ComRuntimeState final {
public:
    struct ServiceKey final {
        ara::com::ServiceIdentifierType service_id;
        ara::com::InstanceIdentifier instance_identifier;

        bool operator<(const ServiceKey& other) const noexcept {
            if (service_id != other.service_id) {
                return service_id < other.service_id;
            }

            return instance_identifier < other.instance_identifier;
        }
    };

    static ComRuntimeState& Instance() noexcept;

    ara::core::Result<void> RegisterInstanceMapping(
        const ara::core::InstanceSpecifier& instance_specifier,
        const ara::com::InstanceIdentifier& instance_identifier,
        const BindingMetadata& metadata) noexcept;

    ara::core::Result<ara::com::InstanceIdentifierContainer> ResolveInstanceIDs(
        const ara::core::InstanceSpecifier& instance_specifier) noexcept;

    ara::core::Result<void> OfferService(
        const ara::com::ServiceIdentifierType& service_id,
        const ara::com::InstanceIdentifier& instance_identifier,
        const BindingMetadata& metadata) noexcept;

    ara::core::Result<void> StopOfferService(
        const ara::com::ServiceIdentifierType& service_id,
        const ara::com::InstanceIdentifier& instance_identifier) noexcept;

    ara::core::Result<ara::core::Vector<BindingMetadata>> FindServices(
        const ara::com::ServiceIdentifierType& service_id,
        const ara::com::InstanceIdentifier& instance_identifier) noexcept;

private:
    struct InstanceMapping final {
        ara::com::InstanceIdentifier instance_identifier;
        BindingMetadata metadata;
    };

    ComRuntimeState() = default;

    IBindingRuntime& GetOrCreateBindingRuntime(BindingType binding_type);

    std::mutex mutex_;
    std::map<std::string, ara::core::Vector<InstanceMapping>, std::less<>> instance_mappings_;
    std::map<BindingType, std::unique_ptr<IBindingRuntime>> binding_runtimes_;
};

} // namespace ara::com::runtime::internal
