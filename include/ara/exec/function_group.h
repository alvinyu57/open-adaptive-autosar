#pragma once

#include "ara/core/instance_specifier.h"

namespace ara::exec {

class FunctionGroup final {
public:
    explicit FunctionGroup(const ara::core::InstanceSpecifier& instance) noexcept;
    FunctionGroup() = delete;
    FunctionGroup(const FunctionGroup& other) = delete;
    FunctionGroup(FunctionGroup&& other) noexcept = default;
    FunctionGroup& operator=(const FunctionGroup& other) = delete;
    FunctionGroup& operator=(FunctionGroup&& other) noexcept = default;
    ~FunctionGroup() noexcept = default;

    bool operator==(const FunctionGroup& other) const noexcept;
    bool operator!=(const FunctionGroup& other) const noexcept;

    const ara::core::InstanceSpecifier& GetInstanceSpecifier() const noexcept;

private:
    ara::core::InstanceSpecifier instance_;
};

} // namespace ara::exec
