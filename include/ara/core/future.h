#pragma once

#include <future>
#include <utility>

#include "ara/core/result.h"

namespace ara::core {

template <typename T, typename E = ErrorCode>
class Future final {
public:
    using value_type = T;
    using result_type = Result<T, E>;

    Future() = default;

    explicit Future(std::future<result_type>&& future) noexcept
        : future_(future.share()) {}

    explicit Future(std::shared_future<result_type> future) noexcept
        : future_(std::move(future)) {}

    bool valid() const noexcept {
        return future_.valid();
    }

    void wait() const {
        future_.wait();
    }

    result_type GetResult() const {
        return future_.get();
    }

    T get() const {
        return GetResult().ValueOrThrow();
    }

private:
    std::shared_future<result_type> future_;
};

template <typename E>
class Future<void, E> final {
public:
    using value_type = void;
    using result_type = Result<void, E>;

    Future() = default;

    explicit Future(std::future<result_type>&& future) noexcept
        : future_(future.share()) {}

    explicit Future(std::shared_future<result_type> future) noexcept
        : future_(std::move(future)) {}

    bool valid() const noexcept {
        return future_.valid();
    }

    void wait() const {
        future_.wait();
    }

    result_type GetResult() const {
        return future_.get();
    }

    void get() const {
        GetResult().ValueOrThrow();
    }

private:
    std::shared_future<result_type> future_;
};

template <typename T, typename E = ErrorCode>
inline Future<T, E> MakeReadyFuture(Result<T, E> result) {
    std::promise<Result<T, E>> promise;
    auto future = promise.get_future();
    promise.set_value(std::move(result));
    return Future<T, E>(std::move(future));
}

template <typename E = ErrorCode>
inline Future<void, E> MakeReadyFuture(Result<void, E> result = Result<void, E>{}) {
    std::promise<Result<void, E>> promise;
    auto future = promise.get_future();
    promise.set_value(std::move(result));
    return Future<void, E>(std::move(future));
}

} // namespace ara::core
