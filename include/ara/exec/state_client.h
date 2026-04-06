#pragma once

#include <functional>

#include "ara/core/future.h"
#include "ara/core/result.h"
#include "ara/exec/exec_error_domain.h"
#include "ara/exec/execution_error_event.h"
#include "ara/exec/function_group_state.h"

namespace ara::exec {

class StateClient final {
public:
    explicit StateClient(
        std::function<void(const ara::exec::ExecutionErrorEvent&)> undefined_state_callback);
    static ara::core::Result<StateClient> Create(
        std::function<void(const ara::exec::ExecutionErrorEvent&)> undefined_state_callback)
        noexcept;
    ~StateClient() noexcept = default;

    StateClient(const StateClient&) = delete;
    StateClient& operator=(const StateClient&) = delete;
    StateClient(StateClient&& rval) noexcept = default;
    StateClient& operator=(StateClient&& rval) noexcept = default;

    ara::core::Future<void> SetState(const FunctionGroupState& state) const noexcept;
    ara::core::Future<void> GetInitialMachineStateTransitionResult() const noexcept;
    ara::core::Result<ExecutionErrorEvent> GetExecutionError(
        const FunctionGroupState& function_group_state) noexcept;

private:
    std::function<void(const ara::exec::ExecutionErrorEvent&)> undefined_state_callback_;
};

} // namespace ara::exec
