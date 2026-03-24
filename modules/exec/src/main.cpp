#include <iostream>

#include "openaa/core/application.hpp"
#include "openaa/exec/execution_manager.hpp"

namespace {

class DiagnosticsApp final : public openaa::core::Application {
public:
    DiagnosticsApp() : openaa::core::Application("diagnostics.stub") {}

private:
    void OnInitialize(openaa::core::RuntimeContext& context) override {
        context.Log().Info(Name(), "Initialize diagnostics service");
        context.Services().Register({
            .service_id = "openaa.exec.diagnostics",
            .endpoint = "local://diagnostics",
            .owner = Name(),
        });
    }

    void OnStart(openaa::core::RuntimeContext& context) override {
        context.Log().Info(Name(), "Diagnostics service ready");
    }

    void OnStop(openaa::core::RuntimeContext& context) override {
        context.Log().Info(Name(), "Diagnostics service stopped");
    }
};

}  // namespace

int main() {
    openaa::core::Logger logger(std::cout);
    openaa::exec::ExecutionManager manager(logger);

    manager.AddApplication(std::make_unique<DiagnosticsApp>());
    manager.Start();
    manager.Stop();

    return 0;
}
