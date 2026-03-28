#pragma once

#include <memory>
#include <string>
#include <vector>

#include "ara/exec/application_manifest.hpp"
#include "ara/core/application.hpp"

namespace ara::exec {

class ExecutionManager {
public:
    virtual ~ExecutionManager() = default;

    virtual void AddApplication(std::unique_ptr<ara::core::Application> application) = 0;
    virtual void RegisterApplicationFactory(
        std::string application_id,
        ara::core::ApplicationFactory factory) = 0;
    virtual void LoadApplicationManifest(const std::string& manifest_path) = 0;
    virtual void Start() = 0;
    virtual void Stop() = 0;

    virtual const ara::core::ServiceRegistry& Services() const = 0;
    virtual std::vector<ApplicationManifest> LoadedManifests() const = 0;
};

} // namespace ara::exec
