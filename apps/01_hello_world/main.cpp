#include <filesystem>
#include <iostream>
#include <sstream>

#include "ara/core/application.hpp"
#include "ara/exec/execution_manager.hpp"
#include "ara/exec/manifest_path.hpp"
#include "openaa/examples/hello_world/hello_world_app.hpp"

int main(int argc, char* argv[]) {
    ara::core::Logger logger(std::cout);
    ara::exec::ExecutionManager manager(logger);

    manager.RegisterApplicationFactory("examples.hello_world",
                                       openaa::examples::hello_world::CreateHelloWorldApp);

    const std::filesystem::path manifest_path =
        argc > 1 ? std::filesystem::path(argv[1])
                 : ara::exec::ResolveManifestPath(argv[0], "hello_world.json");

    manager.LoadApplicationManifest(manifest_path.string());
    manager.Start();

    for (const auto& service : manager.Services().List()) {
        std::ostringstream message;
        message << "Discovered service " << service.service_id << " at " << service.endpoint
                << " owned by " << service.owner;
        logger.Info("example.runner", message.str());
    }

    manager.Stop();
    return 0;
}
