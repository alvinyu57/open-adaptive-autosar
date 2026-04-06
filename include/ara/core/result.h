#pragma once

#include <optional>
#include <utility>

#include "ara/core/error_code.h"

namespace ara::core {

template <typename T, typename E = ErrorCode>
class Result final {
public:
    Result(const T& value)
        : value_(value) {}

    Result(T&& value)
        : value_(std::move(value)) {}

    Result(const E& error)
        : error_(error) {}

    Result(E&& error)
        : error_(std::move(error)) {}

    bool HasValue() const noexcept {
        return value_.has_value();
    }

    explicit operator bool() const noexcept {
        return HasValue();
    }

    T& Value() & {
        if (!value_.has_value()) {
            if (error_.has_value()) {
                error_.value().ThrowAsException();
            }
        }
        return value_.value();
    }

    const T& Value() const& {
        if (!value_.has_value()) {
            if (error_.has_value()) {
                error_.value().ThrowAsException();
            }
        }
        return value_.value();
    }

    E& Error() & noexcept {
        return error_.value();
    }

    const E& Error() const& noexcept {
        return error_.value();
    }

    T ValueOrThrow() const {
        return Value();
    }

private:
    std::optional<T> value_;
    std::optional<E> error_;
};

template <typename E>
class Result<void, E> final {
public:
    Result() = default;

    Result(const E& error)
        : error_(error) {}

    Result(E&& error)
        : error_(std::move(error)) {}

    bool HasValue() const noexcept {
        return !error_.has_value();
    }

    explicit operator bool() const noexcept {
        return HasValue();
    }

    void ValueOrThrow() const {
        if (error_.has_value()) {
            error_.value().ThrowAsException();
        }
    }

    const E& Error() const& noexcept {
        return error_.value();
    }

private:
    std::optional<E> error_;
};

} // namespace ara::core
