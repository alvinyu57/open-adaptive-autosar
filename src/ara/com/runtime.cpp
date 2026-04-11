#include "ara/com/runtime.h"

#include "com_runtime_state.hpp"
#include "shared_memory_ipc.hpp"

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

ara::core::Result<void> PublishEvent(
    std::string_view channel_name,
    std::string_view payload) noexcept {
    return SharedMemoryEventChannel::Publish(channel_name, payload);
}

ara::core::Result<std::optional<ara::core::String>> GetNewEvent(
    std::string_view channel_name,
    std::uint64_t& last_seen_sequence) noexcept {
    return SharedMemoryEventChannel::ReadIfNew(channel_name, last_seen_sequence);
}

ara::core::Result<ara::core::String> CallMethod(
    std::string_view channel_name,
    std::string_view payload,
    std::chrono::milliseconds timeout) noexcept {
    return SharedMemoryMethodChannel::Call(channel_name, payload, timeout);
}

ara::core::Result<std::optional<ChannelMessage>> TakeMethodCall(
    std::string_view channel_name,
    std::uint64_t& last_seen_sequence) noexcept {
    return SharedMemoryMethodChannel::TakeRequest(channel_name, last_seen_sequence);
}

ara::core::Result<void> SendMethodResponse(
    std::string_view channel_name,
    std::uint64_t correlation_id,
    std::string_view payload) noexcept {
    return SharedMemoryMethodChannel::SendResponse(channel_name, correlation_id, payload);
}

ara::core::Result<void> SendFireAndForget(
    std::string_view channel_name,
    std::string_view payload) noexcept {
    return SharedMemoryFireAndForgetChannel::Send(channel_name, payload);
}

ara::core::Result<std::optional<ara::core::String>> TakeFireAndForget(
    std::string_view channel_name,
    std::uint64_t& last_seen_sequence) noexcept {
    return SharedMemoryFireAndForgetChannel::Take(channel_name, last_seen_sequence);
}

} // namespace internal

} // namespace ara::com::runtime
