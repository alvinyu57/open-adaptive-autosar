#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string_view>

#include "ara/com/runtime.h"

namespace ara::com::runtime::internal {

class SharedMemoryEventChannel final {
public:
    static ara::core::Result<void> Publish(
        std::string_view channel_name,
        std::string_view payload) noexcept;

    static ara::core::Result<std::optional<ara::core::String>> ReadIfNew(
        std::string_view channel_name,
        std::uint64_t& last_seen_sequence) noexcept;
};

class SharedMemoryMethodChannel final {
public:
    static ara::core::Result<ara::core::String> Call(
        std::string_view channel_name,
        std::string_view payload,
        std::chrono::milliseconds timeout) noexcept;

    static ara::core::Result<std::optional<ChannelMessage>> TakeRequest(
        std::string_view channel_name,
        std::uint64_t& last_seen_sequence) noexcept;

    static ara::core::Result<void> SendResponse(
        std::string_view channel_name,
        std::uint64_t correlation_id,
        std::string_view payload) noexcept;
};

class SharedMemoryFireAndForgetChannel final {
public:
    static ara::core::Result<void> Send(
        std::string_view channel_name,
        std::string_view payload) noexcept;

    static ara::core::Result<std::optional<ara::core::String>> Take(
        std::string_view channel_name,
        std::uint64_t& last_seen_sequence) noexcept;
};

} // namespace ara::com::runtime::internal
