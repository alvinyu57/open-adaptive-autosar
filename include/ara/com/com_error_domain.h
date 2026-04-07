#pragma once

#include "ara/com/com_fwd.h"
#include "ara/core/error_code.h"

namespace ara::com {

enum class ComErrc : ara::core::ErrorDomain::CodeType {
    kServiceNotAvailable = 1,
    kMaxSamplesExceeded = 2,
    kCommunicationFailure = 3,
    kFieldValueNotInitialized = 6,
    kFieldSetHandlerNotSet = 7,
    kServiceNotOffered = 11,
    kInstanceIDNotResolvable = 15,
    kMaxSampleCountNotRealizable = 16,
    kUnknownApplicationError = 22,
    kMinimumSendIntervalViolation = 23,
};

class ComException : public ara::core::Exception {
public:
    explicit ComException(ara::core::ErrorCode error_code) noexcept;
};

class ComErrorDomain final : public ara::core::ErrorDomain {
public:
    using Errc = ComErrc;
    using Exception = ComException;

    static constexpr IdType kId{0x8000000000001260ULL};

    ComErrorDomain() noexcept;

    const char* Name() const noexcept override;
    const char* Message(CodeType error_code) const noexcept override;
    void ThrowAsException(const ara::core::ErrorCode& error_code) const override;
};

const ara::core::ErrorDomain& GetComErrorDomain() noexcept;

inline ara::core::ErrorCode MakeErrorCode(
    ComErrc code,
    ara::core::ErrorDomain::SupportDataType data = 0) noexcept {
    return ara::core::ErrorCode(
        static_cast<ara::core::ErrorDomain::CodeType>(code),
        GetComErrorDomain(),
        data);
}

} // namespace ara::com

namespace ara::core {

template <>
struct is_error_code_enum<ara::com::ComErrc> : std::true_type {};

} // namespace ara::core
