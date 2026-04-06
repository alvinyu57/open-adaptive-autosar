#pragma once

#include <string_view>

#include "ara/core/core_error_domain.h"
#include "ara/core/result.h"
#include "ara/core/string.h"

namespace ara::core {

class InstanceSpecifier final {
public:
    InstanceSpecifier() = delete;

    explicit InstanceSpecifier(std::string_view meta_model_path)
        : meta_model_path_(meta_model_path) {
        if (meta_model_path.empty()) {
            throw CoreException(MakeErrorCode(CoreErrc::kInvalidArgument));
        }
    }

    static Result<InstanceSpecifier> Create(std::string_view meta_model_path) noexcept {
        if (meta_model_path.empty()) {
            return Result<InstanceSpecifier>{MakeErrorCode(CoreErrc::kInvalidArgument)};
        }

        return Result<InstanceSpecifier>{InstanceSpecifier(meta_model_path)};
    }

    const String& ToString() const noexcept {
        return meta_model_path_;
    }

    std::string_view View() const noexcept {
        return meta_model_path_.View();
    }

private:
    String meta_model_path_;
};

inline bool operator==(
    const InstanceSpecifier& lhs,
    const InstanceSpecifier& rhs) noexcept {
    return lhs.View() == rhs.View();
}

inline bool operator!=(
    const InstanceSpecifier& lhs,
    const InstanceSpecifier& rhs) noexcept {
    return !(lhs == rhs);
}

} // namespace ara::core
