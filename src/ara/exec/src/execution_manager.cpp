#include "ara/exec/execution_manager.hpp"

#include <sstream>
#include <stdexcept>
#include <utility>

#include "application_manifest_loader.hpp"

namespace ara::exec {

ExecutionManager::ExecutionManager(ara::core::Logger& logger)
    : logger_(&logger) {}

void ExecutionManager::AddApplication(std::unique_ptr<ara::core::Application> application) {
    logger_->Info("exec", "Register application: " + application->Name());
    applications_.push_back({
        .manifest =
            {
                .application_id = application->Name(),
                .short_name = application->Name(),
                .executable = application->Name(),
            },
        .application = std::move(application),
    });
}

void ExecutionManager::RegisterApplicationFactory(
    std::string application_id,
    ara::core::ApplicationFactory factory) {
    if (application_id.empty()) {
        throw std::invalid_argument("application factory id must not be empty");
    }

    const auto [it, inserted] =
        application_factories_.emplace(std::move(application_id), std::move(factory));

    if (!inserted) {
        throw std::logic_error("application factory already registered");
    }
}

void ExecutionManager::LoadApplicationManifest(const std::string& manifest_path) {
    const auto manifest = LoadApplicationManifestFromFile(manifest_path);
    const auto factory_it = application_factories_.find(manifest.application_id);
    if (factory_it == application_factories_.end()) {
        throw std::runtime_error(
            "no application factory registered for '" + manifest.application_id + "'");
    }

    auto application = factory_it->second();
    if (application == nullptr) {
        throw std::runtime_error(
            "application factory returned null for '" + manifest.application_id + "'");
    }

    if (application->Name() != manifest.short_name) {
        throw std::runtime_error(
            "manifest shortName '" + manifest.short_name +
            "' does not match application name '" + application->Name() + "'");
    }

    logger_->Info("exec", "Loaded manifest: " + manifest_path);
    applications_.push_back({
        .manifest = manifest,
        .application = std::move(application),
    });
}

void ExecutionManager::Start() {
    ara::core::RuntimeContext context(registry_, *logger_);
    logger_->Info("exec", "Execution manager start");

    for (auto& managed_application : applications_) {
        if (!managed_application.manifest.auto_start) {
            logger_->Info(
                "exec",
                "Skip autostart for " + managed_application.manifest.short_name);
            continue;
        }

        logger_->Info("exec", "Initialize " + managed_application.application->Name());
        managed_application.application->Initialize(context);

        logger_->Info("exec", "Start " + managed_application.application->Name());
        managed_application.application->Run(context);
    }

    std::ostringstream summary;
    const auto services = registry_.List();
    summary << "Runtime online with " << services.size() << " service(s)";
    logger_->Info("exec", summary.str());
}

void ExecutionManager::Stop() {
    ara::core::RuntimeContext context(registry_, *logger_);

    for (auto it = applications_.rbegin(); it != applications_.rend(); ++it) {
        logger_->Info("exec", "Stop " + it->application->Name());
        it->application->Stop(context);
    }

    logger_->Info("exec", "Execution manager stopped");
}

const ara::core::ServiceRegistry& ExecutionManager::Services() const {
    return registry_;
}

std::vector<ApplicationManifest> ExecutionManager::LoadedManifests() const {
    std::vector<ApplicationManifest> manifests;
    manifests.reserve(applications_.size());

    for (const auto& application : applications_) {
        manifests.push_back(application.manifest);
    }

    return manifests;
}

} // namespace ara::exec
