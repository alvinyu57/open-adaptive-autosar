#pragma once

#include <memory>
#include <utility>

#include "ara/com/com_fwd.h"

namespace ara::com::e2e {

enum class ProfileCheckStatus : unsigned char {
    kNotChecked = 0,
};

} // namespace ara::com::e2e

namespace ara::com {

template <typename T>
class SamplePtr final {
public:
    using element_type = T;

    constexpr SamplePtr(std::nullptr_t other = nullptr) noexcept
        : ptr_(other) {}

    explicit SamplePtr(T* other) noexcept
        : ptr_(other) {}

    SamplePtr(const SamplePtr&) = delete;
    SamplePtr& operator=(const SamplePtr&) = delete;

    SamplePtr(SamplePtr&& other) noexcept = default;
    SamplePtr& operator=(SamplePtr&& other) noexcept = default;

    ~SamplePtr() noexcept = default;

    T* Get() const noexcept {
        return ptr_.get();
    }

    ara::com::e2e::ProfileCheckStatus GetProfileCheckStatus() const noexcept {
        return ara::com::e2e::ProfileCheckStatus::kNotChecked;
    }

    void Reset(std::nullptr_t other) noexcept {
        ptr_.reset(other);
    }

    void Swap(SamplePtr& other) noexcept {
        ptr_.swap(other.ptr_);
    }

    explicit operator bool() const noexcept {
        return static_cast<bool>(ptr_);
    }

    T& operator*() const noexcept {
        return *ptr_;
    }

    T* operator->() const noexcept {
        return ptr_.get();
    }

    SamplePtr& operator=(std::nullptr_t other) noexcept {
        Reset(other);
        return *this;
    }

private:
    std::unique_ptr<T> ptr_;
};

template <typename T>
inline void swap(SamplePtr<T>& lhs, SamplePtr<T>& rhs) noexcept {
    lhs.Swap(rhs);
}

} // namespace ara::com
