#pragma once

#include <chrono>
#include <optional>

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
    kSomeIp = 2,
};

struct BindingMetadata final {
    BindingType binding_type{BindingType::kIpc};
    ara::core::String endpoint;
};

struct ChannelMessage final {
    std::uint64_t correlation_id{0U};
    ara::core::String payload;
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

ara::core::Result<void> PublishEvent(
    BindingType binding,
    std::string_view channel_name,
    std::string_view payload) noexcept;

ara::core::Result<std::optional<ara::core::String>> GetNewEvent(
    BindingType binding,
    std::string_view channel_name,
    std::uint64_t& last_seen_sequence) noexcept;

ara::core::Result<ara::core::String> CallMethod(
    BindingType binding,
    std::string_view channel_name,
    std::string_view payload,
    std::chrono::milliseconds timeout) noexcept;

ara::core::Result<std::optional<ChannelMessage>> TakeMethodCall(
    BindingType binding,
    std::string_view channel_name,
    std::uint64_t& last_seen_sequence) noexcept;

ara::core::Result<void> SendMethodResponse(
    BindingType binding,
    std::string_view channel_name,
    std::uint64_t correlation_id,
    std::string_view payload) noexcept;

ara::core::Result<void> SendFireAndForget(
    BindingType binding,
    std::string_view channel_name,
    std::string_view payload) noexcept;

ara::core::Result<std::optional<ara::core::String>> TakeFireAndForget(
    BindingType binding,
    std::string_view channel_name,
    std::uint64_t& last_seen_sequence) noexcept;

} // namespace internal

} // namespace ara::com::runtime
