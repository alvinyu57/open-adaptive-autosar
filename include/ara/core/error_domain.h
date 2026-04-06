#pragma once

#include <cstdint>
#include <exception>

#include "ara/core/core_fwd.h"

namespace ara::core {

class ErrorDomain {
public:
    using IdType = ErrorDomainIdType;
    using CodeType = std::int32_t;
    using SupportDataType = std::uint32_t;

    constexpr explicit ErrorDomain(IdType value) noexcept
        : id_(value) {}

    ErrorDomain(const ErrorDomain&) = delete;
    ErrorDomain(ErrorDomain&&) = delete;
    ErrorDomain& operator=(const ErrorDomain&) = delete;
    ErrorDomain& operator=(ErrorDomain&&) = delete;

    virtual ~ErrorDomain() noexcept = default;

    constexpr IdType Id() const noexcept {
        return id_;
    }

    virtual const char* Name() const noexcept = 0;
    virtual const char* Message(CodeType error_code) const noexcept = 0;
    virtual void ThrowAsException(const ErrorCode& error_code) const = 0;

private:
    IdType id_;
};

constexpr bool operator==(const ErrorDomain& lhs, const ErrorDomain& rhs) noexcept {
    return lhs.Id() == rhs.Id();
}

constexpr bool operator!=(const ErrorDomain& lhs, const ErrorDomain& rhs) noexcept {
    return !(lhs == rhs);
}

} // namespace ara::core
