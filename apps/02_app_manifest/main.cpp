#include <filesystem>
#include <iostream>
#include <sstream>

#include "openaa/apps/app_manifest/app_manifest_app.hpp"
#include "openaa/core/application.hpp"
#include "openaa/exec/execution_manager.hpp"
#include "openaa/exec/manifest_path.hpp"

int main(int argc, char* argv[]) {
    openaa::core::Logger logger(std::cout);
    openaa::exec::ExecutionManager manager(logger);

    manager.RegisterApplicationFactory(
        "apps.02_app_manifest.demo",
        openaa::apps::app_manifest::CreateAppManifestApp);

    const std::filesystem::path manifest_path =
        argc > 1 ? std::filesystem::path(argv[1])
                 : openaa::exec::ResolveManifestPath(argv[0], "application_manifest.json");

    manager.LoadApplicationManifest(manifest_path.string());
    manager.Start();

    for (const auto& service : manager.Services().List()) {
        std::ostringstream message;
        message << "Discovered service " << service.service_id << " at " << service.endpoint
                << " owned by " << service.owner;
        logger.Info("app_manifest.runner", message.str());
    }

    manager.Stop();
    return 0;
}
