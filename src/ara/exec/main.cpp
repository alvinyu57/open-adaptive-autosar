#include <filesystem>
#include <iostream>
#include <memory>

#include "ara/core/initialization.h"
#include "ara/exec/application.hpp"
#include "ara/exec/execution_manager.hpp"
#include "ara/exec/manifest_path.hpp"
#include "ara/log/logger.hpp"

namespace {

class DiagnosticsApp final : public ara::exec::Application {
public:
    DiagnosticsApp()
        : ara::exec::Application("diagnostics.stub") {}

private:
    void OnInitialize(ara::exec::RuntimeContext& context) override {
        context.Log().Info(Name(), "Initialize diagnostics service");
        context.Services().Register({
            .service_id = "ara.exec.diagnostics",
            .endpoint = "local://diagnostics",
            .owner = Name(),
        });
    }

    void OnStart(ara::exec::RuntimeContext& context) override {
        context.Log().Info(Name(), "Diagnostics service ready");
    }

    void OnStop(ara::exec::RuntimeContext& context) override {
        context.Log().Info(Name(), "Diagnostics service stopped");
    }
};

} // namespace

int main(int argc, char* argv[]) {
    const auto init_result = ara::core::Initialize(argc, argv);
    if (!init_result.HasValue()) {
        return 1;
    }

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

    const auto deinit_result = ara::core::Deinitialize();
    return deinit_result.HasValue() ? 0 : 1;
}
