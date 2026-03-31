#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "ara/exec/application.hpp"
#include "ara/exec/application_manifest.hpp"
#include "ara/log/logger.hpp"

namespace ara::exec {

class ExecutionManager {
public:
    explicit ExecutionManager(ara::log::Logger& logger);
    virtual ~ExecutionManager() = default;

    virtual void AddApplication(std::unique_ptr<ara::exec::Application> application);
    virtual void RegisterApplicationFactory(
        std::string application_id,
        ara::exec::ApplicationFactory factory);
    virtual void LoadApplicationManifest(const std::string& manifest_path);
    virtual void Start();
    virtual void Stop();

    virtual const ara::exec::ServiceRegistry& Services() const;
    virtual std::vector<ApplicationManifest> LoadedManifests() const;

private:
    struct ManagedApplication {
        ApplicationManifest manifest;
        std::unique_ptr<ara::exec::Application> application;
    };

    ara::log::Logger* logger_;
    ara::exec::ServiceRegistry registry_;
    std::unordered_map<std::string, ara::exec::ApplicationFactory> application_factories_;
    std::vector<ManagedApplication> applications_;
};

} // namespace ara::exec
