#include "ara/core/core_error_domain.h"

namespace ara::core {

CoreErrorDomain::CoreErrorDomain() noexcept
    : ErrorDomain(kId) {}

const char* CoreErrorDomain::Name() const noexcept {
    return "Core";
}

const char* CoreErrorDomain::Message(CodeType error_code) const noexcept {
    switch (static_cast<CoreErrc>(error_code)) {
        case CoreErrc::kInvalidArgument:
            return "invalid argument";
        case CoreErrc::kInvalidState:
            return "invalid state";
        case CoreErrc::kOutOfRange:
            return "out of range";
        case CoreErrc::kNoSuchElement:
            return "no such element";
        case CoreErrc::kAlreadyExists:
            return "already exists";
        default:
            return "unknown core error";
    }
}

void CoreErrorDomain::ThrowAsException(const ErrorCode& error_code) const {
    throw CoreException(error_code);
}

const CoreErrorDomain& GetCoreErrorDomain() noexcept {
    static const CoreErrorDomain domain;
    return domain;
}

} // namespace ara::core
