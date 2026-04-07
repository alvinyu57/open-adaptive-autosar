#pragma once

#include <cstdint>
#include <functional>
#include <memory>

#include "ara/com/service/find_service_handle.h"
#include "ara/com/service/instance_identifier.h"
#include "ara/core/vector.h"

namespace ara::com {

template <typename T>
using SampleAllocateePtr = std::unique_ptr<T>;

template <typename T>
using ServiceHandleContainer = ara::core::Vector<T>;

using InstanceIdentifierContainer = ara::core::Vector<InstanceIdentifier>;

template <typename T>
using FindServiceHandler = std::function<void(ServiceHandleContainer<T>, FindServiceHandle)>;

enum class MethodCallProcessingMode : std::uint8_t {
    kPoll = 0,
    kEvent = 1,
    kEventSingleThread = 2,
};

enum class ServiceState : std::uint8_t {
    kNotAvailable = 0,
    kAvailable = 1,
};

using ServiceStateHandler = std::function<void(ServiceState)>;

enum class SubscriptionState : std::uint8_t {
    kSubscribed = 0,
    kNotSubscribed = 1,
    kSubscriptionPending = 2,
};

using SubscriptionStateChangeHandler = std::function<void(SubscriptionState)>;
using TriggerReceiveHandler = std::function<void()>;

} // namespace ara::com
