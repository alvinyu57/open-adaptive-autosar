#include <filesystem>
#include <iostream>
#include <sstream>

#include "ara/core/initialization.h"
#include "ara/exec/execution_manager.hpp"
#include "ara/exec/manifest_path.hpp"
#include "ara/log/logger.hpp"
#include "openaa/apps/app_manifest/app_manifest_app.hpp"

int main(int argc, char* argv[]) {
    if (!ara::core::Initialize(argc, argv).HasValue()) {
        return 1;
    }

    ara::log::Logger logger(std::cout);
    ara::exec::ExecutionManager manager(logger);

    manager.RegisterApplicationFactory("apps.02_app_manifest.demo",
                                       openaa::apps::app_manifest::CreateAppManifestApp);

    const std::filesystem::path manifest_path =
        argc > 1 ? std::filesystem::path(argv[1])
                 : ara::exec::ResolveManifestPath(argv[0], "application_manifest.json");

    manager.LoadApplicationManifest(manifest_path.string());
    manager.Start();

    for (const auto& service : manager.Services().List()) {
        std::ostringstream message;
        message << "Discovered service " << service.service_id << " at " << service.endpoint
                << " owned by " << service.owner;
        logger.Info("app_manifest.runner", message.str());
    }

    manager.Stop();
    return ara::core::Deinitialize().HasValue() ? 0 : 1;
}
