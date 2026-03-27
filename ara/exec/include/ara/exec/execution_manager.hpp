#pragma once

#include <memory>

#include "ara/core/application.hpp"

namespace ara::exec {

class ExecutionManager {
public:
    virtual ~ExecutionManager() = default;

    virtual void AddApplication(std::unique_ptr<ara::core::Application> application) = 0;
    virtual void Start() = 0;
    virtual void Stop() = 0;

    virtual const ara::core::ServiceRegistry& Services() const = 0;
};

} // namespace ara::exec
