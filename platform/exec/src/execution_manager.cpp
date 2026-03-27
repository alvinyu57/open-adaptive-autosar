#include "openaa/exec/execution_manager.hpp"

#include <sstream>

namespace openaa::exec {

ExecutionManager::ExecutionManager(ara::core::Logger& logger)
    : logger_(&logger) {}

void ExecutionManager::AddApplication(std::unique_ptr<ara::core::Application> application) {
    logger_->Info("exec", "Register application: " + application->Name());
    applications_.push_back(std::move(application));
}

void ExecutionManager::Start() {
    ara::core::RuntimeContext context(registry_, *logger_);
    logger_->Info("exec", "Execution manager start");

    for (auto& application : applications_) {
        logger_->Info("exec", "Initialize " + application->Name());
        application->Initialize(context);

        logger_->Info("exec", "Start " + application->Name());
        application->Run(context);
    }

    std::ostringstream summary;
    const auto services = registry_.List();
    summary << "Runtime online with " << services.size() << " service(s)";
    logger_->Info("exec", summary.str());
}

void ExecutionManager::Stop() {
    ara::core::RuntimeContext context(registry_, *logger_);

    for (auto it = applications_.rbegin(); it != applications_.rend(); ++it) {
        logger_->Info("exec", "Stop " + (*it)->Name());
        (*it)->Stop(context);
    }

    logger_->Info("exec", "Execution manager stopped");
}

const ara::core::ServiceRegistry& ExecutionManager::Services() const {
    return registry_;
}

} // namespace openaa::exec
