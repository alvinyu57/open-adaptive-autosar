#pragma once

#include "ara/core/error_code.h"
#include "ara/exec/exec_fwd.h"

namespace ara::exec {

enum class ExecErrc : ara::core::ErrorDomain::CodeType {
    kNoCommunication = 3,
    kInvalidMetaModelIdentifier = 4,
    kOperationCanceled = 5,
    kOperationFailed = 6,
    kInvalidTransition = 9,
    kIntegrityOrAuthenticityCheckFailed = 14,
    kUnexpectedTermination = 15,
    kInvalidArgument = 16,
};

class ExecException final : public ara::core::Exception {
public:
    explicit ExecException(ara::core::ErrorCode error_code) noexcept;
};

class ExecErrorDomain final : public ara::core::ErrorDomain {
public:
    static constexpr IdType kId{0x8000000000000202ULL};

    ExecErrorDomain() noexcept;

    const char* Name() const noexcept override;
    const char* Message(CodeType error_code) const noexcept override;
    void ThrowAsException(const ara::core::ErrorCode& error_code) const override;
};

const ara::core::ErrorDomain& GetExecErrorDomain() noexcept;

inline ara::core::ErrorCode MakeErrorCode(
    ExecErrc code,
    ara::core::ErrorDomain::SupportDataType data = 0) noexcept {
    return ara::core::ErrorCode(
        static_cast<ara::core::ErrorDomain::CodeType>(code),
        GetExecErrorDomain(),
        data);
}

} // namespace ara::exec

namespace ara::core {

template <>
struct is_error_code_enum<ara::exec::ExecErrc> : std::true_type {};

} // namespace ara::core
