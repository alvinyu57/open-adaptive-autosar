#pragma once

#include <string_view>

#include "ara/core/string.h"
#include "ara/exec/function_group.h"

namespace ara::exec {

class FunctionGroupState final {
public:
    FunctionGroupState(const FunctionGroup& function_group, std::string_view state) noexcept;
    FunctionGroupState(const FunctionGroupState& other) noexcept = default;
    FunctionGroupState(FunctionGroupState&& other) noexcept = default;
    FunctionGroupState& operator=(const FunctionGroupState& other) & noexcept = default;
    FunctionGroupState& operator=(FunctionGroupState&& other) & noexcept = default;
    ~FunctionGroupState() noexcept = default;

    bool operator==(const FunctionGroupState& other) const noexcept;
    bool operator!=(const FunctionGroupState& other) const noexcept;

    const FunctionGroup& GetFunctionGroup() const noexcept;
    std::string_view GetState() const noexcept;

private:
    FunctionGroup function_group_;
    ara::core::String state_;
};

} // namespace ara::exec
