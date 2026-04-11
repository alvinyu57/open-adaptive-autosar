#pragma once

#include <string_view>

#include "ara/com/com_fwd.h"
#include "ara/core/abort.h"
#include "ara/core/string.h"

namespace ara::com {

class ServiceVersionType final {
public:
    ServiceVersionType(const ServiceVersionType&) = default;
    ServiceVersionType(ServiceVersionType&&) noexcept = default;
    ServiceVersionType& operator=(const ServiceVersionType&) = default;
    ServiceVersionType& operator=(ServiceVersionType&&) noexcept = default;
    ~ServiceVersionType() = default;

    explicit ServiceVersionType(std::string_view value)
        : value_(Validate(value)) {}

    bool operator<(const ServiceVersionType& other) const {
        return value_.View() < other.value_.View();
    }

    bool operator==(const ServiceVersionType& other) const {
        return value_ == other.value_;
    }

    ara::core::String::StringView ToString() const {
        return value_.View();
    }

private:
    static std::string_view Validate(std::string_view value) {
        if (value.empty()) {
            ara::core::Abort("ara::com::ServiceVersionType requires a non-empty value");
        }

        return value;
    }

    ara::core::String value_;
};

inline bool operator!=(const ServiceVersionType& lhs, const ServiceVersionType& rhs) {
    return !(lhs == rhs);
}

} // namespace ara::com
