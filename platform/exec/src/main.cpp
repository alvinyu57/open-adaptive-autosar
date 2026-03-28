#include <filesystem>
#include <iostream>

#include "ara/core/application.hpp"
#include "openaa/core/application.hpp"
#include "openaa/exec/execution_manager.hpp"
#include "openaa/exec/manifest_path.hpp"

namespace {

class DiagnosticsApp final : public openaa::core::Application {
public:
    DiagnosticsApp()
        : openaa::core::Application("diagnostics.stub") {}

private:
    void OnInitialize(ara::core::RuntimeContext& context) override {
        context.Log().Info(Name(), "Initialize diagnostics service");
        context.Services().Register({
            .service_id = "openaa.exec.diagnostics",
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
    openaa::core::Logger logger(std::cout);
    openaa::exec::ExecutionManager manager(logger);

    manager.RegisterApplicationFactory("diagnostics.stub", []() {
        return std::make_unique<DiagnosticsApp>();
    });

    const std::filesystem::path manifest_path =
        argc > 1 ? std::filesystem::path(argv[1])
                 : openaa::exec::ResolveManifestPath(argv[0], "diagnostics.json");

    manager.LoadApplicationManifest(manifest_path.string());
    manager.Start();
    manager.Stop();

    return 0;
}
