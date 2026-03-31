#pragma once

#include <cstdint>
#include <functional>

#include "ara/core/result.h"
#include "ara/exec/exec_error_domain.h"
#include "ara/exec/exec_fwd.h"

namespace ara::exec {

enum class ExecutionState : std::uint32_t {
    kRunning = 0U,
};

class ExecutionClient final {
public:
    explicit ExecutionClient(std::function<void()> termination_handler);
    static ara::core::Result<ExecutionClient> Create(
        std::function<void()> termination_handler) noexcept;
    ~ExecutionClient() noexcept;

    ExecutionClient(const ExecutionClient&) = delete;
    ExecutionClient& operator=(const ExecutionClient&) = delete;
    ExecutionClient(ExecutionClient&& rval) noexcept = default;
    ExecutionClient& operator=(ExecutionClient&& rval) noexcept = default;

    ara::core::Result<void> ReportExecutionState(ExecutionState state) noexcept;

private:
    std::function<void()> termination_handler_;
    bool owns_sigterm_handler_{false};
};

} // namespace ara::exec
