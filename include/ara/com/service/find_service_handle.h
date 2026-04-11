#pragma once

#include <cstdint>

namespace ara::com {

struct FindServiceHandle final {
    FindServiceHandle() = delete;

    explicit FindServiceHandle(std::uint64_t value) noexcept
        : value(value) {}

    FindServiceHandle(const FindServiceHandle&) = default;
    FindServiceHandle(FindServiceHandle&&) noexcept = default;
    FindServiceHandle& operator=(const FindServiceHandle&) = default;
    FindServiceHandle& operator=(FindServiceHandle&&) noexcept = default;
    ~FindServiceHandle() = default;

    bool operator<(const FindServiceHandle& other) const {
        return value < other.value;
    }

    bool operator==(const FindServiceHandle& other) const {
        return value == other.value;
    }

    std::uint64_t value;
};

inline bool operator!=(const FindServiceHandle& lhs, const FindServiceHandle& rhs) {
    return !(lhs == rhs);
}

} // namespace ara::com
