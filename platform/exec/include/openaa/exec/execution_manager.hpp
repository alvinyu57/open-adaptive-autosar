#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "ara/exec/application_manifest.hpp"
#include "ara/exec/execution_manager.hpp"
#include "openaa/core/application.hpp"

namespace openaa::exec {

class ExecutionManager final : public ara::exec::ExecutionManager {
public:
    explicit ExecutionManager(ara::core::Logger& logger);

    void AddApplication(std::unique_ptr<ara::core::Application> application) override;
    void RegisterApplicationFactory(
        std::string application_id,
        ara::core::ApplicationFactory factory) override;
    void LoadApplicationManifest(const std::string& manifest_path) override;
    void Start() override;
    void Stop() override;

    const ara::core::ServiceRegistry& Services() const override;
    std::vector<ara::exec::ApplicationManifest> LoadedManifests() const override;

private:
    struct ManagedApplication {
        ara::exec::ApplicationManifest manifest;
        std::unique_ptr<ara::core::Application> application;
    };

    ara::core::Logger* logger_;
    openaa::core::ServiceRegistry registry_;
    std::unordered_map<std::string, ara::core::ApplicationFactory> application_factories_;
    std::vector<ManagedApplication> applications_;
};

} // namespace openaa::exec
