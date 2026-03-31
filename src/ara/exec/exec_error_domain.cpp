#include "ara/exec/exec_error_domain.h"

namespace ara::exec {

ExecException::ExecException(ara::core::ErrorCode error_code) noexcept
    : ara::core::Exception(error_code) {}

ExecErrorDomain::ExecErrorDomain() noexcept
    : ara::core::ErrorDomain(kId) {}

const char* ExecErrorDomain::Name() const noexcept {
    return "Exec";
}

const char* ExecErrorDomain::Message(CodeType error_code) const noexcept {
    switch (static_cast<ExecErrc>(error_code)) {
    case ExecErrc::kNoCommunication:
        return "communication error";
    case ExecErrc::kInvalidMetaModelIdentifier:
        return "invalid meta model identifier";
    case ExecErrc::kOperationCanceled:
        return "operation canceled";
    case ExecErrc::kOperationFailed:
        return "operation failed";
    case ExecErrc::kInvalidTransition:
        return "invalid transition";
    case ExecErrc::kIntegrityOrAuthenticityCheckFailed:
        return "integrity or authenticity check failed";
    case ExecErrc::kUnexpectedTermination:
        return "unexpected termination";
    case ExecErrc::kInvalidArgument:
        return "invalid argument";
    default:
        return "unknown execution management error";
    }
}

void ExecErrorDomain::ThrowAsException(const ara::core::ErrorCode& error_code) const {
    throw ExecException(error_code);
}

const ara::core::ErrorDomain& GetExecErrorDomain() noexcept {
    static const ExecErrorDomain domain;
    return domain;
}

} // namespace ara::exec
