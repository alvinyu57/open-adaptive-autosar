#pragma once

#include <string_view>

#include "ara/core/abort.h"
#include "ara/core/string.h"

namespace ara::com {

class InstanceIdentifier final {
public:
    InstanceIdentifier(const InstanceIdentifier&) noexcept = default;
    InstanceIdentifier(InstanceIdentifier&&) noexcept = default;
    InstanceIdentifier& operator=(const InstanceIdentifier&) noexcept = default;
    InstanceIdentifier& operator=(InstanceIdentifier&&) noexcept = default;
    ~InstanceIdentifier() noexcept = default;

    explicit InstanceIdentifier(std::string_view serialized_format)
        : serialized_format_(Validate(serialized_format)) {}

    static InstanceIdentifier Create(std::string_view serialized_format) noexcept {
        return InstanceIdentifier(Validate(serialized_format));
    }

    bool operator<(const InstanceIdentifier& other) const noexcept {
        return serialized_format_.View() < other.serialized_format_.View();
    }

    bool operator==(const InstanceIdentifier& other) const noexcept {
        return serialized_format_ == other.serialized_format_;
    }

    std::string_view toString() const noexcept {
        return serialized_format_.View();
    }

private:
    static std::string_view Validate(std::string_view serialized_format) noexcept {
        if (serialized_format.empty()) {
            ara::core::Abort("ara::com::InstanceIdentifier requires a non-empty value");
        }

        return serialized_format;
    }

    ara::core::String serialized_format_;
};

inline bool operator!=(const InstanceIdentifier& lhs, const InstanceIdentifier& rhs) noexcept {
    return !(lhs == rhs);
}

} // namespace ara::com
