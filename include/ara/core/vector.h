#pragma once

#include <initializer_list>
#include <utility>
#include <vector>

#include "ara/core/abort.h"
#include "ara/core/core_fwd.h"

namespace ara::core {

template <typename T, typename Allocator = std::allocator<T>>
class Vector final {
public:
    using value_type = T;
    using allocator_type = Allocator;
    using size_type = typename std::vector<T, Allocator>::size_type;
    using difference_type = typename std::vector<T, Allocator>::difference_type;
    using reference = typename std::vector<T, Allocator>::reference;
    using const_reference = typename std::vector<T, Allocator>::const_reference;
    using pointer = typename std::vector<T, Allocator>::pointer;
    using const_pointer = typename std::vector<T, Allocator>::const_pointer;
    using iterator = typename std::vector<T, Allocator>::iterator;
    using const_iterator = typename std::vector<T, Allocator>::const_iterator;

    Vector() = default;

    explicit Vector(const Allocator& allocator)
        : data_(allocator) {}

    explicit Vector(size_type count, const Allocator& allocator = Allocator())
        : data_(count, allocator) {}

    Vector(size_type count, const T& value, const Allocator& allocator = Allocator())
        : data_(count, value, allocator) {}

    template <typename InputIt>
    Vector(InputIt first, InputIt last, const Allocator& allocator = Allocator())
        : data_(first, last, allocator) {}

    Vector(std::initializer_list<T> init, const Allocator& allocator = Allocator())
        : data_(init, allocator) {}

    Vector(const Vector&) = default;
    Vector(Vector&&) noexcept = default;
    Vector& operator=(const Vector&) = default;
    Vector& operator=(Vector&&) noexcept = default;
    ~Vector() = default;

    iterator begin() noexcept { return data_.begin(); }
    const_iterator begin() const noexcept { return data_.begin(); }
    const_iterator cbegin() const noexcept { return data_.cbegin(); }

    iterator end() noexcept { return data_.end(); }
    const_iterator end() const noexcept { return data_.end(); }
    const_iterator cend() const noexcept { return data_.cend(); }

    bool empty() const noexcept { return data_.empty(); }
    size_type size() const noexcept { return data_.size(); }
    size_type capacity() const noexcept { return data_.capacity(); }

    void reserve(size_type new_capacity) { data_.reserve(new_capacity); }
    void shrink_to_fit() { data_.shrink_to_fit(); }
    void clear() noexcept { data_.clear(); }

    reference operator[](size_type pos) noexcept { return data_[pos]; }
    const_reference operator[](size_type pos) const noexcept { return data_[pos]; }

    reference at(size_type pos) {
        if (pos >= data_.size()) {
            Abort("ara::core::Vector::at out of range: ", pos);
        }
        return data_[pos];
    }

    const_reference at(size_type pos) const {
        if (pos >= data_.size()) {
            Abort("ara::core::Vector::at out of range: ", pos);
        }
        return data_[pos];
    }

    reference front() {
        if (data_.empty()) {
            Abort("ara::core::Vector::front on empty vector");
        }
        return data_.front();
    }

    const_reference front() const {
        if (data_.empty()) {
            Abort("ara::core::Vector::front on empty vector");
        }
        return data_.front();
    }

    reference back() {
        if (data_.empty()) {
            Abort("ara::core::Vector::back on empty vector");
        }
        return data_.back();
    }

    const_reference back() const {
        if (data_.empty()) {
            Abort("ara::core::Vector::back on empty vector");
        }
        return data_.back();
    }

    pointer data() noexcept { return data_.data(); }
    const_pointer data() const noexcept { return data_.data(); }

    void push_back(const T& value) { data_.push_back(value); }
    void push_back(T&& value) { data_.push_back(std::move(value)); }

    template <typename... Args>
    reference emplace_back(Args&&... args) {
        return data_.emplace_back(std::forward<Args>(args)...);
    }

    void pop_back() {
        if (data_.empty()) {
            Abort("ara::core::Vector::pop_back on empty vector");
        }
        data_.pop_back();
    }

    void resize(size_type count) { data_.resize(count); }
    void resize(size_type count, const value_type& value) { data_.resize(count, value); }

    iterator insert(const_iterator pos, const T& value) { return data_.insert(pos, value); }
    iterator insert(const_iterator pos, T&& value) { return data_.insert(pos, std::move(value)); }
    iterator insert(const_iterator pos, size_type count, const T& value) {
        return data_.insert(pos, count, value);
    }

    template <typename InputIt>
    iterator insert(const_iterator pos, InputIt first, InputIt last) {
        return data_.insert(pos, first, last);
    }

    iterator insert(const_iterator pos, std::initializer_list<T> init) {
        return data_.insert(pos, init);
    }

    template <typename... Args>
    iterator emplace(const_iterator pos, Args&&... args) {
        return data_.emplace(pos, std::forward<Args>(args)...);
    }

    iterator erase(const_iterator pos) { return data_.erase(pos); }
    iterator erase(const_iterator first, const_iterator last) { return data_.erase(first, last); }

    void swap(Vector& other) noexcept(noexcept(data_.swap(other.data_))) {
        data_.swap(other.data_);
    }

    const std::vector<T, Allocator>& Native() const noexcept { return data_; }
    std::vector<T, Allocator>& Native() noexcept { return data_; }

private:
    std::vector<T, Allocator> data_;
};

template <typename T, typename Allocator>
inline bool operator==(const Vector<T, Allocator>& lhs, const Vector<T, Allocator>& rhs) {
    return lhs.Native() == rhs.Native();
}

template <typename T, typename Allocator>
inline bool operator!=(const Vector<T, Allocator>& lhs, const Vector<T, Allocator>& rhs) {
    return !(lhs == rhs);
}

template <typename T, typename Allocator>
inline void swap(Vector<T, Allocator>& lhs, Vector<T, Allocator>& rhs) noexcept(noexcept(lhs.swap(rhs))) {
    lhs.swap(rhs);
}

} // namespace ara::core
