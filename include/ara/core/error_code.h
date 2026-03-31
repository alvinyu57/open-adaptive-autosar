#pragma once

#include <string>
#include <type_traits>

#include "ara/core/core_fwd.h"
#include "ara/core/error_domain.h"

namespace ara::core {

template <typename EnumT>
struct is_error_code_enum : std::false_type {};

template <typename EnumT>
inline constexpr bool is_error_code_enum_v = is_error_code_enum<EnumT>::value;

class ErrorCode final {
public:
    using CodeType = ErrorDomain::CodeType;
    using SupportDataType = ErrorDomain::SupportDataType;

    constexpr ErrorCode(CodeType value, const ErrorDomain& domain,
                        SupportDataType support_data = 0) noexcept
        : value_(value)
        , support_data_(support_data)
        , domain_(&domain) {}

    template <typename EnumT,
              typename std::enable_if_t<is_error_code_enum_v<EnumT>, int> = 0>
    constexpr ErrorCode(EnumT value, SupportDataType support_data = 0) noexcept
        : ErrorCode(MakeErrorCode(value, support_data)) {}

    constexpr CodeType Value() const noexcept {
        return value_;
    }

    constexpr SupportDataType SupportData() const noexcept {
        return support_data_;
    }

    constexpr const ErrorDomain& Domain() const noexcept {
        return *domain_;
    }

    const char* Message() const noexcept {
        return domain_->Message(value_);
    }

    void ThrowAsException() const {
        domain_->ThrowAsException(*this);
    }

private:
    CodeType value_;
    SupportDataType support_data_;
    const ErrorDomain* domain_;
};

inline constexpr bool operator==(const ErrorCode& lhs, const ErrorCode& rhs) noexcept {
    return lhs.Value() == rhs.Value() && lhs.Domain() == rhs.Domain();
}

inline constexpr bool operator!=(const ErrorCode& lhs, const ErrorCode& rhs) noexcept {
    return !(lhs == rhs);
}

class Exception : public std::exception {
public:
    explicit Exception(ErrorCode error_code) noexcept
        : error_code_(error_code) {}

    const char* what() const noexcept override {
        return error_code_.Message();
    }

    const ErrorCode& Error() const noexcept {
        return error_code_;
    }

private:
    ErrorCode error_code_;
};

} // namespace ara::core
