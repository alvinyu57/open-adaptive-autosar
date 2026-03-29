#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "ara/core/application.hpp"
#include "ara/exec/application_manifest.hpp"

namespace ara::exec {

class ExecutionManager {
public:
    explicit ExecutionManager(ara::core::Logger& logger);
    virtual ~ExecutionManager() = default;

    virtual void AddApplication(std::unique_ptr<ara::core::Application> application);
    virtual void RegisterApplicationFactory(
        std::string application_id,
        ara::core::ApplicationFactory factory);
    virtual void LoadApplicationManifest(const std::string& manifest_path);
    virtual void Start();
    virtual void Stop();

    virtual const ara::core::ServiceRegistry& Services() const;
    virtual std::vector<ApplicationManifest> LoadedManifests() const;

private:
    struct ManagedApplication {
        ApplicationManifest manifest;
        std::unique_ptr<ara::core::Application> application;
    };

    ara::core::Logger* logger_;
    ara::core::ServiceRegistry registry_;
    std::unordered_map<std::string, ara::core::ApplicationFactory> application_factories_;
    std::vector<ManagedApplication> applications_;
};

} // namespace ara::exec
