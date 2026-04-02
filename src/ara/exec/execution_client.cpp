#include "ara/exec/execution_client.h"

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <string>
#include <thread>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <utility>

#include "em_runtime_state.hpp"

namespace ara::exec {
namespace {

constexpr char kExecutionReportSocketEnv[] = "OPENAA_EXEC_REPORT_SOCKET";
constexpr char kExecutionReportProcessEnv[] = "OPENAA_EXEC_PROCESS_NAME";

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
    const char* socket_path = std::getenv(kExecutionReportSocketEnv);
    const char* process_name = std::getenv(kExecutionReportProcessEnv);
    if (socket_path != nullptr && process_name != nullptr) {
        if (state != ExecutionState::kRunning) {
            return ara::core::Result<void>{MakeErrorCode(ExecErrc::kInvalidArgument)};
        }

        const int socket_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
        if (socket_fd == -1) {
            return ara::core::Result<void>{MakeErrorCode(ExecErrc::kNoCommunication)};
        }

        sockaddr_un address{};
        address.sun_family = AF_UNIX;
        std::snprintf(address.sun_path, sizeof(address.sun_path), "%s", socket_path);

        const std::string payload = "pid=" + std::to_string(getpid()) + ";process=" +
                                    process_name + ";state=Running";
        const auto result = sendto(socket_fd,
                                   payload.data(),
                                   payload.size(),
                                   0,
                                   reinterpret_cast<const sockaddr*>(&address),
                                   sizeof(address));
        close(socket_fd);
        if (result == -1) {
            return ara::core::Result<void>{MakeErrorCode(ExecErrc::kNoCommunication)};
        }

        return ara::core::Result<void>{};
    }

    return internal::RuntimeState::Instance().ReportExecutionState(state);
}

} // namespace ara::exec
