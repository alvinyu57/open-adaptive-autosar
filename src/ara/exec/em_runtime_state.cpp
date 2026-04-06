#include "em_runtime_state.hpp"

namespace ara::exec::internal {

RuntimeState& RuntimeState::Instance() noexcept {
    static RuntimeState runtime_state;
    return runtime_state;
}

ara::core::Result<void> RuntimeState::ReportExecutionState(ExecutionState state) noexcept {
    std::scoped_lock lock(mutex_);

    if (state != ExecutionState::kRunning) {
        return ara::core::Result<void>{MakeErrorCode(ExecErrc::kInvalidArgument)};
    }

    if (execution_running_reported_) {
        return ara::core::Result<void>{MakeErrorCode(ExecErrc::kInvalidTransition)};
    }

    execution_running_reported_ = true;
    return ara::core::Result<void>{};
}

ara::core::Future<void> RuntimeState::SetState(const FunctionGroupState& state) noexcept {
    std::scoped_lock lock(mutex_);

    if (state.GetState().empty()) {
        return ara::core::MakeReadyFuture(
            ara::core::Result<void>{MakeErrorCode(ExecErrc::kInvalidArgument)});
    }

    auto& record = function_groups_[MakeKey(state.GetFunctionGroup())];
    record.current_state = std::string(state.GetState());
    record.is_undefined = false;
    record.last_execution_error = 0U;

    return ara::core::MakeReadyFuture(ara::core::Result<void>{});
}

ara::core::Future<void> RuntimeState::GetInitialMachineStateTransitionResult() noexcept {
    return ara::core::MakeReadyFuture(ara::core::Result<void>{});
}

ara::core::Result<ExecutionErrorEvent>
RuntimeState::GetExecutionError(const FunctionGroupState& function_group_state) noexcept {
    std::scoped_lock lock(mutex_);

    const auto it = function_groups_.find(MakeKey(function_group_state.GetFunctionGroup()));
    if (it == function_groups_.end() || !it->second.is_undefined) {
        return ara::core::Result<ExecutionErrorEvent>{MakeErrorCode(ExecErrc::kOperationFailed)};
    }

    return ara::core::Result<ExecutionErrorEvent>{ExecutionErrorEvent{
        .executionError = it->second.last_execution_error,
        .functionGroup = FunctionGroup(ara::core::InstanceSpecifier(
            function_group_state.GetFunctionGroup().GetInstanceSpecifier().View())),
    }};
}

std::string RuntimeState::MakeKey(const FunctionGroup& function_group) {
    return std::string(function_group.GetInstanceSpecifier().View());
}

} // namespace ara::exec::internal
