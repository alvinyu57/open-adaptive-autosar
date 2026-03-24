#pragma once

#include <memory>
#include <vector>

#include "openaa/core/application.hpp"

namespace openaa::exec {

class ExecutionManager {
  public:
    explicit ExecutionManager(openaa::core::Logger &logger);

    void AddApplication(std::unique_ptr<openaa::core::Application> application);
    void Start();
    void Stop();

    const openaa::core::ServiceRegistry &Services() const;

  private:
    openaa::core::Logger *logger_;
    openaa::core::ServiceRegistry registry_;
    std::vector<std::unique_ptr<openaa::core::Application>> applications_;
};

} // namespace openaa::exec
