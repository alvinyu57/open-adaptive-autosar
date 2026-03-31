#include "ara/exec/function_group.h"

namespace ara::exec {

FunctionGroup::FunctionGroup(const ara::core::InstanceSpecifier& instance) noexcept
    : instance_(instance) {}

bool FunctionGroup::operator==(const FunctionGroup& other) const noexcept {
    return instance_ == other.instance_;
}

bool FunctionGroup::operator!=(const FunctionGroup& other) const noexcept {
    return !(*this == other);
}

const ara::core::InstanceSpecifier& FunctionGroup::GetInstanceSpecifier() const noexcept {
    return instance_;
}

} // namespace ara::exec
