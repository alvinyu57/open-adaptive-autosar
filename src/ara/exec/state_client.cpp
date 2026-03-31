#include "ara/exec/state_client.h"

#include <utility>

#include "em_runtime_state.hpp"

namespace ara::exec {

StateClient::StateClient(
    std::function<void(const ara::exec::ExecutionErrorEvent&)> undefined_state_callback)
    : undefined_state_callback_(std::move(undefined_state_callback)) {
    if (!undefined_state_callback_) {
        throw ExecException(MakeErrorCode(ExecErrc::kInvalidArgument));
    }
}

ara::core::Result<StateClient> StateClient::Create(
    std::function<void(const ara::exec::ExecutionErrorEvent&)> undefined_state_callback)
    noexcept {
    if (!undefined_state_callback) {
        return ara::core::Result<StateClient>{
            MakeErrorCode(ExecErrc::kInvalidArgument)};
    }

    try {
        return ara::core::Result<StateClient>{
            StateClient(std::move(undefined_state_callback))};
    } catch (const ExecException& exception) {
        return ara::core::Result<StateClient>{exception.Error()};
    } catch (...) {
        return ara::core::Result<StateClient>{
            MakeErrorCode(ExecErrc::kNoCommunication)};
    }
}

ara::core::Future<void> StateClient::SetState(const FunctionGroupState& state) const noexcept {
    return internal::RuntimeState::Instance().SetState(state);
}

ara::core::Future<void> StateClient::GetInitialMachineStateTransitionResult() const noexcept {
    return internal::RuntimeState::Instance().GetInitialMachineStateTransitionResult();
}

ara::core::Result<ExecutionErrorEvent> StateClient::GetExecutionError(
    const FunctionGroupState& function_group_state) noexcept {
    return internal::RuntimeState::Instance().GetExecutionError(function_group_state);
}

} // namespace ara::exec
