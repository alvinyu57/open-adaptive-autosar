#pragma once

#include <memory>
#include <vector>

#include "ara/exec/execution_manager.hpp"
#include "openaa/platform/core/application.hpp"

namespace openaa::platform::exec {

class ExecutionManager final : public ara::exec::ExecutionManager {
  public:
    explicit ExecutionManager(ara::core::Logger &logger);

    void AddApplication(std::unique_ptr<ara::core::Application> application) override;
    void Start() override;
    void Stop() override;

    const ara::core::ServiceRegistry &Services() const override;

  private:
    ara::core::Logger *logger_;
    openaa::platform::core::ServiceRegistry registry_;
    std::vector<std::unique_ptr<ara::core::Application>> applications_;
};

} // namespace openaa::platform::exec
