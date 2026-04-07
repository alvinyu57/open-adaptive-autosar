#include "ara/com/com_error_domain.h"

namespace ara::com {

ComException::ComException(ara::core::ErrorCode error_code) noexcept
    : ara::core::Exception(error_code) {}

ComErrorDomain::ComErrorDomain() noexcept
    : ara::core::ErrorDomain(kId) {}

const char* ComErrorDomain::Name() const noexcept {
    return "Com";
}

const char* ComErrorDomain::Message(CodeType error_code) const noexcept {
    switch (static_cast<ComErrc>(error_code)) {
        case ComErrc::kServiceNotAvailable:
            return "service not available";
        case ComErrc::kMaxSamplesExceeded:
            return "max samples exceeded";
        case ComErrc::kCommunicationFailure:
            return "communication failure";
        case ComErrc::kFieldValueNotInitialized:
            return "field value not initialized";
        case ComErrc::kFieldSetHandlerNotSet:
            return "field set handler not set";
        case ComErrc::kServiceNotOffered:
            return "service not offered";
        case ComErrc::kInstanceIDNotResolvable:
            return "instance identifier not resolvable";
        case ComErrc::kMaxSampleCountNotRealizable:
            return "max sample count not realizable";
        case ComErrc::kUnknownApplicationError:
            return "unknown application error";
        case ComErrc::kMinimumSendIntervalViolation:
            return "minimum send interval violation";
        default:
            return "unknown communication management error";
    }
}

void ComErrorDomain::ThrowAsException(const ara::core::ErrorCode& error_code) const {
    throw ComException(error_code);
}

const ara::core::ErrorDomain& GetComErrorDomain() noexcept {
    static const ComErrorDomain domain;
    return domain;
}

} // namespace ara::com
