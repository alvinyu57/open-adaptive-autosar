#pragma once

#include <initializer_list>
#include <string>
#include <string_view>
#include <utility>

#include "ara/core/abort.h"
#include "ara/core/core_fwd.h"

namespace ara::core {

template <typename CharT, typename Traits = std::char_traits<CharT>,
          typename Allocator = std::allocator<CharT>>
class BasicString {
public:
    using value_type = CharT;
    using traits_type = Traits;
    using allocator_type = Allocator;
    using size_type = typename std::basic_string<CharT, Traits, Allocator>::size_type;
    using difference_type = typename std::basic_string<CharT, Traits, Allocator>::difference_type;
    using reference = typename std::basic_string<CharT, Traits, Allocator>::reference;
    using const_reference =
        typename std::basic_string<CharT, Traits, Allocator>::const_reference;
    using pointer = typename std::basic_string<CharT, Traits, Allocator>::pointer;
    using const_pointer = typename std::basic_string<CharT, Traits, Allocator>::const_pointer;
    using iterator = typename std::basic_string<CharT, Traits, Allocator>::iterator;
    using const_iterator = typename std::basic_string<CharT, Traits, Allocator>::const_iterator;
    using StringView = std::basic_string_view<CharT, Traits>;

    BasicString() = default;

    explicit BasicString(const Allocator& allocator)
        : data_(allocator) {}

    BasicString(const CharT* s, const Allocator& allocator = Allocator())
        : data_(s, allocator) {}

    BasicString(const CharT* s, size_type count, const Allocator& allocator = Allocator())
        : data_(s, count, allocator) {}

    BasicString(size_type count, CharT ch, const Allocator& allocator = Allocator())
        : data_(count, ch, allocator) {}

    BasicString(StringView view, const Allocator& allocator = Allocator())
        : data_(view.data(), view.size(), allocator) {}

    BasicString(const std::basic_string<CharT, Traits, Allocator>& value)
        : data_(value) {}

    BasicString(std::basic_string<CharT, Traits, Allocator>&& value) noexcept
        : data_(std::move(value)) {}

    template <typename InputIt>
    BasicString(InputIt first, InputIt last, const Allocator& allocator = Allocator())
        : data_(first, last, allocator) {}

    BasicString(std::initializer_list<CharT> init, const Allocator& allocator = Allocator())
        : data_(init, allocator) {}

    BasicString(const BasicString&) = default;
    BasicString(BasicString&&) noexcept = default;
    BasicString& operator=(const BasicString&) = default;
    BasicString& operator=(BasicString&&) noexcept = default;
    ~BasicString() = default;

    BasicString& operator=(const CharT* value) {
        data_ = value;
        return *this;
    }

    BasicString& operator=(StringView value) {
        data_.assign(value.data(), value.size());
        return *this;
    }

    iterator begin() noexcept { return data_.begin(); }
    const_iterator begin() const noexcept { return data_.begin(); }
    const_iterator cbegin() const noexcept { return data_.cbegin(); }

    iterator end() noexcept { return data_.end(); }
    const_iterator end() const noexcept { return data_.end(); }
    const_iterator cend() const noexcept { return data_.cend(); }

    bool empty() const noexcept { return data_.empty(); }
    size_type size() const noexcept { return data_.size(); }
    size_type length() const noexcept { return data_.length(); }
    size_type capacity() const noexcept { return data_.capacity(); }

    void reserve(size_type new_capacity) { data_.reserve(new_capacity); }
    void shrink_to_fit() { data_.shrink_to_fit(); }
    void clear() noexcept { data_.clear(); }

    const CharT* c_str() const noexcept { return data_.c_str(); }
    const CharT* data() const noexcept { return data_.data(); }
    CharT* data() noexcept { return data_.data(); }

    reference operator[](size_type pos) noexcept { return data_[pos]; }
    const_reference operator[](size_type pos) const noexcept { return data_[pos]; }

    reference at(size_type pos) {
        if (pos >= data_.size()) {
            Abort("ara::core::BasicString::at out of range: ", pos);
        }
        return data_[pos];
    }

    const_reference at(size_type pos) const {
        if (pos >= data_.size()) {
            Abort("ara::core::BasicString::at out of range: ", pos);
        }
        return data_[pos];
    }

    reference front() {
        if (data_.empty()) {
            Abort("ara::core::BasicString::front on empty string");
        }
        return data_.front();
    }

    const_reference front() const {
        if (data_.empty()) {
            Abort("ara::core::BasicString::front on empty string");
        }
        return data_.front();
    }

    reference back() {
        if (data_.empty()) {
            Abort("ara::core::BasicString::back on empty string");
        }
        return data_.back();
    }

    const_reference back() const {
        if (data_.empty()) {
            Abort("ara::core::BasicString::back on empty string");
        }
        return data_.back();
    }

    BasicString& append(StringView view) {
        data_.append(view.data(), view.size());
        return *this;
    }

    BasicString& append(const BasicString& value) {
        data_.append(value.data_);
        return *this;
    }

    BasicString& push_back(CharT ch) {
        data_.push_back(ch);
        return *this;
    }

    BasicString& operator+=(StringView view) { return append(view); }

    BasicString& operator+=(const BasicString& value) { return append(value); }

    BasicString& operator+=(CharT ch) {
        data_.push_back(ch);
        return *this;
    }

    int compare(StringView view) const noexcept {
        return StringView(data_).compare(view);
    }

    BasicString substr(size_type pos = 0, size_type count = StringView::npos) const {
        if (pos > data_.size()) {
            Abort("ara::core::BasicString::substr out of range: ", pos);
        }
        return BasicString(data_.substr(pos, count));
    }

    StringView View() const noexcept {
        return StringView(data_);
    }

    operator StringView() const noexcept {
        return View();
    }

    const std::basic_string<CharT, Traits, Allocator>& Native() const noexcept { return data_; }
    std::basic_string<CharT, Traits, Allocator>& Native() noexcept { return data_; }

private:
    std::basic_string<CharT, Traits, Allocator> data_;
};

template <typename CharT, typename Traits, typename Allocator>
inline bool operator==(const BasicString<CharT, Traits, Allocator>& lhs,
                       const BasicString<CharT, Traits, Allocator>& rhs) noexcept {
    return lhs.View() == rhs.View();
}

template <typename CharT, typename Traits, typename Allocator>
inline bool operator!=(const BasicString<CharT, Traits, Allocator>& lhs,
                       const BasicString<CharT, Traits, Allocator>& rhs) noexcept {
    return !(lhs == rhs);
}

template <typename CharT, typename Traits, typename Allocator>
inline bool operator==(const BasicString<CharT, Traits, Allocator>& lhs,
                       std::basic_string_view<CharT, Traits> rhs) noexcept {
    return lhs.View() == rhs;
}

template <typename CharT, typename Traits, typename Allocator>
inline bool operator==(std::basic_string_view<CharT, Traits> lhs,
                       const BasicString<CharT, Traits, Allocator>& rhs) noexcept {
    return lhs == rhs.View();
}

template <typename CharT, typename Traits, typename Allocator>
inline BasicString<CharT, Traits, Allocator> operator+(
    const BasicString<CharT, Traits, Allocator>& lhs,
    const BasicString<CharT, Traits, Allocator>& rhs) {
    BasicString<CharT, Traits, Allocator> result(lhs);
    result += rhs;
    return result;
}

class String final : public BasicString<char> {
public:
    using BasicString<char>::BasicString;

    String(const BasicString<char>& value)
        : BasicString<char>(value) {}

    String(BasicString<char>&& value) noexcept
        : BasicString<char>(std::move(value)) {}
};

} // namespace ara::core
