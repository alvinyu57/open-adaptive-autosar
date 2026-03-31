#include "ara/core/initialization.h"

#include <atomic>

#include "ara/core/core_error_domain.h"

namespace ara::core {

namespace {

std::atomic<bool>& IsInitialized() {
    static std::atomic<bool> initialized{false};
    return initialized;
}

} // namespace

Result<void> Initialize() noexcept {
    bool expected{false};
    if (!IsInitialized().compare_exchange_strong(expected, true)) {
        return MakeErrorCode(CoreErrc::kInvalidState);
    }

    return {};
}

Result<void> Initialize(int, char*[]) noexcept {
    return Initialize();
}

Result<void> Deinitialize() noexcept {
    bool expected{true};
    if (!IsInitialized().compare_exchange_strong(expected, false)) {
        return MakeErrorCode(CoreErrc::kInvalidState);
    }

    return {};
}

} // namespace ara::core
