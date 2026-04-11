#pragma once

#include <string_view>

#include "ara/com/com_fwd.h"
#include "ara/core/abort.h"
#include "ara/core/string.h"

namespace ara::com {

class ServiceIdentifierType final {
public:
    ServiceIdentifierType(const ServiceIdentifierType&) = default;
    ServiceIdentifierType(ServiceIdentifierType&&) noexcept = default;
    ServiceIdentifierType& operator=(const ServiceIdentifierType&) = default;
    ServiceIdentifierType& operator=(ServiceIdentifierType&&) noexcept = default;
    ~ServiceIdentifierType() = default;

    explicit ServiceIdentifierType(std::string_view value)
        : value_(Validate(value)) {}

    bool operator<(const ServiceIdentifierType& other) const {
        return value_.View() < other.value_.View();
    }

    bool operator==(const ServiceIdentifierType& other) const {
        return value_ == other.value_;
    }

    ara::core::String::StringView toString() const {
        return value_.View();
    }

private:
    static std::string_view Validate(std::string_view value) {
        if (value.empty()) {
            ara::core::Abort("ara::com::ServiceIdentifierType requires a non-empty value");
        }

        return value;
    }

    ara::core::String value_;
};

inline bool operator!=(const ServiceIdentifierType& lhs, const ServiceIdentifierType& rhs) {
    return !(lhs == rhs);
}

} // namespace ara::com
