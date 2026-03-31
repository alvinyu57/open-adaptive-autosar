#include "ara/exec/execution_client.h"

#include <csignal>
#include <mutex>
#include <thread>
#include <utility>

#include "em_runtime_state.hpp"

namespace ara::exec {
namespace {

std::mutex g_sigterm_mutex;
std::function<void()>* g_sigterm_callback{nullptr};
std::atomic<bool> g_sigterm_handler_installed{false};

void SigtermHandler(int) {
    std::function<void()> callback;
    {
        std::scoped_lock lock(g_sigterm_mutex);
        if (g_sigterm_callback != nullptr) {
            callback = *g_sigterm_callback;
        }
    }

    if (callback) {
        std::thread worker(std::move(callback));
        worker.detach();
    }
}

} // namespace

ExecutionClient::ExecutionClient(std::function<void()> termination_handler)
    : termination_handler_(std::move(termination_handler)) {
    if (!termination_handler_) {
        throw ExecException(MakeErrorCode(ExecErrc::kInvalidArgument));
    }

    std::scoped_lock lock(g_sigterm_mutex);
    g_sigterm_callback = &termination_handler_;
    std::signal(SIGTERM, SigtermHandler);
    g_sigterm_handler_installed.store(true);
    owns_sigterm_handler_ = true;
}

ara::core::Result<ExecutionClient> ExecutionClient::Create(
    std::function<void()> termination_handler) noexcept {
    if (!termination_handler) {
        return ara::core::Result<ExecutionClient>{
            MakeErrorCode(ExecErrc::kInvalidArgument)};
    }

    try {
        return ara::core::Result<ExecutionClient>{
            ExecutionClient(std::move(termination_handler))};
    } catch (const ExecException& exception) {
        return ara::core::Result<ExecutionClient>{exception.Error()};
    } catch (...) {
        return ara::core::Result<ExecutionClient>{
            MakeErrorCode(ExecErrc::kNoCommunication)};
    }
}

ExecutionClient::~ExecutionClient() noexcept {
    if (!owns_sigterm_handler_) {
        return;
    }

    std::scoped_lock lock(g_sigterm_mutex);
    if (g_sigterm_callback == &termination_handler_) {
        std::signal(SIGTERM, SIG_DFL);
        g_sigterm_callback = nullptr;
        g_sigterm_handler_installed.store(false);
    }
}

ara::core::Result<void> ExecutionClient::ReportExecutionState(ExecutionState state) noexcept {
    return internal::RuntimeState::Instance().ReportExecutionState(state);
}

} // namespace ara::exec
