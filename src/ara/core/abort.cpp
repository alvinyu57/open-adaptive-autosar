#include "ara/core/abort.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <mutex>
#include <vector>

namespace ara::core {

namespace {

std::mutex& AbortHandlerMutex() {
    static std::mutex mutex;
    return mutex;
}

std::vector<AbortHandler>& AbortHandlers() {
    static std::vector<AbortHandler> handlers;
    return handlers;
}

} // namespace

bool AddAbortHandler(AbortHandler handler) noexcept {
    if (handler == nullptr) {
        return false;
    }

    std::scoped_lock lock(AbortHandlerMutex());
    auto& handlers = AbortHandlers();
    const auto duplicate = std::find(handlers.begin(), handlers.end(), handler);
    if (duplicate != handlers.end()) {
        return false;
    }

    handlers.push_back(handler);
    return true;
}

namespace detail {

[[noreturn]] void AbortImpl(std::string_view message) noexcept {
    std::vector<AbortHandler> local_handlers;

    {
        std::scoped_lock lock(AbortHandlerMutex());
        local_handlers = AbortHandlers();
    }

    for (const auto handler : local_handlers) {
        if (handler != nullptr) {
            handler(message);
        }
    }

    std::abort();
}

} // namespace detail

} // namespace ara::core
