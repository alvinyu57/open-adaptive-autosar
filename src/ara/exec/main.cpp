#include <filesystem>
#include <iostream>
#include <memory>

#include "ara/core/application.hpp"
#include "ara/exec/execution_manager.hpp"
#include "ara/exec/manifest_path.hpp"
#include "ara/log/logger.hpp"

namespace {

class DiagnosticsApp final : public ara::core::Application {
public:
    DiagnosticsApp()
        : ara::core::Application("diagnostics.stub") {}

private:
    void OnInitialize(ara::core::RuntimeContext& context) override {
        context.Log().Info(Name(), "Initialize diagnostics service");
        context.Services().Register({
            .service_id = "ara.exec.diagnostics",
            .endpoint = "local://diagnostics",
            .owner = Name(),
        });
    }

    void OnStart(ara::core::RuntimeContext& context) override {
        context.Log().Info(Name(), "Diagnostics service ready");
    }

    void OnStop(ara::core::RuntimeContext& context) override {
        context.Log().Info(Name(), "Diagnostics service stopped");
    }
};

} // namespace

int main(int argc, char* argv[]) {
    ara::log::Logger logger(std::cout);
    ara::exec::ExecutionManager manager(logger);

    manager.RegisterApplicationFactory("diagnostics.stub", []() {
        return std::make_unique<DiagnosticsApp>();
    });

    const std::filesystem::path manifest_path =
        argc > 1 ? std::filesystem::path(argv[1])
                 : ara::exec::ResolveManifestPath(argv[0], "diagnostics.json");

    manager.LoadApplicationManifest(manifest_path.string());
    manager.Start();
    manager.Stop();

    return 0;
}
