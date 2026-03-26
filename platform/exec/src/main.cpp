#include <iostream>

#include "ara/core/application.hpp"
#include "openaa/core/application.hpp"
#include "openaa/exec/execution_manager.hpp"

namespace {

class DiagnosticsApp final : public openaa::core::Application {
  public:
    DiagnosticsApp() : openaa::core::Application("diagnostics.stub") {}

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
    openaa::core::Logger logger(std::cout);
    openaa::exec::ExecutionManager manager(logger);

    manager.AddApplication(std::make_unique<DiagnosticsApp>());
    manager.Start();
    manager.Stop();

    return 0;
}
