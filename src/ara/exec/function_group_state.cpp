#include "ara/exec/function_group_state.h"

namespace ara::exec {

FunctionGroupState::FunctionGroupState(
    const FunctionGroup& function_group,
    std::string_view state) noexcept
    : function_group_(ara::core::InstanceSpecifier(function_group.GetInstanceSpecifier().View()))
    , state_(state) {}

bool FunctionGroupState::operator==(const FunctionGroupState& other) const noexcept {
    return function_group_ == other.function_group_ && state_ == other.state_;
}

bool FunctionGroupState::operator!=(const FunctionGroupState& other) const noexcept {
    return !(*this == other);
}

const FunctionGroup& FunctionGroupState::GetFunctionGroup() const noexcept {
    return function_group_;
}

std::string_view FunctionGroupState::GetState() const noexcept {
    return state_.View();
}

} // namespace ara::exec
