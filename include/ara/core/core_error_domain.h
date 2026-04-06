#pragma once

#include "ara/core/error_code.h"

namespace ara::core {

enum class CoreErrc : ErrorDomain::CodeType {
    kInvalidArgument = 1,
    kInvalidState = 2,
    kOutOfRange = 3,
    kNoSuchElement = 4,
    kAlreadyExists = 5,
};

class CoreException : public Exception {
public:
    using Exception::Exception;
};

class CoreErrorDomain final : public ErrorDomain {
public:
    constexpr static IdType kId{0x8000000000000000ULL};

    CoreErrorDomain() noexcept;

    const char* Name() const noexcept override;
    const char* Message(CodeType error_code) const noexcept override;
    void ThrowAsException(const ErrorCode& error_code) const override;
};

const CoreErrorDomain& GetCoreErrorDomain() noexcept;

inline ErrorCode MakeErrorCode(
    CoreErrc code,
    ErrorDomain::SupportDataType support_data = 0) noexcept {
    return ErrorCode(
        static_cast<ErrorDomain::CodeType>(code),
        GetCoreErrorDomain(),
        support_data);
}

template <>
struct is_error_code_enum<CoreErrc> : std::true_type {};

} // namespace ara::core
