#include <iostream>
#include <memory>
#include <stdexcept>

#include "ara/core/application.hpp"
#include "openaa/platform/core/application.hpp"
#include "openaa/platform/exec/execution_manager.hpp"

namespace {

class TestApp final : public openaa::platform::core::Application {
  public:
    TestApp() : openaa::platform::core::Application("tests.smoke") {}

  private:
    void OnInitialize(ara::core::RuntimeContext &context) override {
        const bool inserted = context.Services().Register({
            .service_id = "tests.smoke.service",
            .endpoint = "local://tests-smoke",
            .owner = Name(),
        });

        if (!inserted) {
            throw std::runtime_error("service registration failed");
        }
    }

    void OnStart(ara::core::RuntimeContext &) override {}
    void OnStop(ara::core::RuntimeContext &) override {}
};

} // namespace

int main() {
    openaa::platform::core::Logger logger(std::cout);
    openaa::platform::exec::ExecutionManager manager(logger);

    manager.AddApplication(std::make_unique<TestApp>());
    manager.Start();

    const auto services = manager.Services().List();
    if (services.size() != 1 || services.front().service_id != "tests.smoke.service") {
        std::cerr << "unexpected service registry state\n";
        return 1;
    }

    manager.Stop();
    return 0;
}
