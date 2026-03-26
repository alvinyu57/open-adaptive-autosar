#include <iostream>

#include "ara/core/application.hpp"
#include "openaa/platform/core/application.hpp"
#include "openaa/platform/exec/execution_manager.hpp"

namespace {

class DiagnosticsApp final : public openaa::platform::core::Application {
  public:
    DiagnosticsApp() : openaa::platform::core::Application("diagnostics.stub") {}

  private:
    void OnInitialize(ara::core::RuntimeContext &context) override {
        context.Log().Info(Name(), "Initialize diagnostics service");
        context.Services().Register({
            .service_id = "openaa.exec.diagnostics",
            .endpoint = "local://diagnostics",
            .owner = Name(),
        });
    }

    void OnStart(ara::core::RuntimeContext &context) override {
        context.Log().Info(Name(), "Diagnostics service ready");
    }

    void OnStop(ara::core::RuntimeContext &context) override {
        context.Log().Info(Name(), "Diagnostics service stopped");
    }
};

} // namespace

int main() {
    openaa::platform::core::Logger logger(std::cout);
    openaa::platform::exec::ExecutionManager manager(logger);

    manager.AddApplication(std::make_unique<DiagnosticsApp>());
    manager.Start();
    manager.Stop();

    return 0;
}
