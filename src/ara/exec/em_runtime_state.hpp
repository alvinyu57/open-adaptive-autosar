#pragma once

#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>

#include "ara/core/future.h"
#include "ara/core/result.h"
#include "ara/exec/execution_client.h"
#include "ara/exec/execution_error_event.h"
#include "ara/exec/function_group_state.h"

namespace ara::exec::internal {

class RuntimeState final {
public:
    static RuntimeState& Instance() noexcept;

    ara::core::Result<void> ReportExecutionState(ExecutionState state) noexcept;
    ara::core::Future<void> SetState(const FunctionGroupState& state) noexcept;
    ara::core::Future<void> GetInitialMachineStateTransitionResult() noexcept;
    ara::core::Result<ExecutionErrorEvent> GetExecutionError(
        const FunctionGroupState& function_group_state) noexcept;

private:
    struct FunctionGroupRecord {
        std::string current_state{"Off"};
        bool is_undefined{false};
        ExecutionError last_execution_error{};
    };

    RuntimeState() = default;

    static std::string MakeKey(const FunctionGroup& function_group);

    mutable std::mutex mutex_;
    bool execution_running_reported_{false};
    std::unordered_map<std::string, FunctionGroupRecord> function_groups_;
};

} // namespace ara::exec::internal
