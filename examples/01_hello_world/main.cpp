#include <iostream>
#include <sstream>

#include "openaa/examples/hello_world/hello_world_app.hpp"
#include "openaa/core/application.hpp"
#include "openaa/exec/execution_manager.hpp"

int main() {
    openaa::core::Logger logger(std::cout);
    openaa::exec::ExecutionManager manager(logger);

    manager.AddApplication(openaa::examples::hello_world::CreateHelloWorldApp());
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
