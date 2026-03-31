#pragma once

#include <sstream>
#include <string>
#include <string_view>
#include <utility>

namespace ara::core {

using AbortHandler = void (*)(std::string_view message) noexcept;

bool AddAbortHandler(AbortHandler handler) noexcept;

namespace detail {

[[noreturn]] void AbortImpl(std::string_view message) noexcept;

inline std::string BuildAbortMessage() {
    return {};
}

template <typename... Args>
std::string BuildAbortMessage(Args&&... args) {
    std::ostringstream stream;
    (stream << ... << std::forward<Args>(args));
    return stream.str();
}

} // namespace detail

template <typename... Args>
[[noreturn]] void Abort(Args&&... args) noexcept {
    try {
        detail::AbortImpl(detail::BuildAbortMessage(std::forward<Args>(args)...));
    } catch (...) {
        detail::AbortImpl("ara::core::Abort");
    }
}

} // namespace ara::core
