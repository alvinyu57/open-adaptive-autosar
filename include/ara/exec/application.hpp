#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "ara/log/logger.hpp"

namespace ara::exec {

struct ServiceEntry {
    std::string service_id;
    std::string endpoint;
    std::string owner;
};

class ServiceRegistry {
public:
    bool Register(ServiceEntry entry);
    std::vector<ServiceEntry> List() const;

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, ServiceEntry> services_;
};

class RuntimeContext {
public:
    RuntimeContext(ServiceRegistry& services, ara::log::Logger& logger) noexcept
        : services_(&services)
        , logger_(&logger) {}

    ServiceRegistry& Services() const noexcept { return *services_; }
    ara::log::Logger& Log() const noexcept { return *logger_; }

private:
    ServiceRegistry* services_;
    ara::log::Logger* logger_;
};

enum class ApplicationState {
    kCreated,
    kInitialized,
    kRunning,
    kStopped,
};

class Application {
public:
    explicit Application(std::string name);
    virtual ~Application() = default;

    void Initialize(RuntimeContext& context);
    void Run(RuntimeContext& context);
    void Stop(RuntimeContext& context);

    const std::string& Name() const;
    ApplicationState State() const;

protected:
    virtual void OnInitialize(RuntimeContext& context) = 0;
    virtual void OnStart(RuntimeContext& context) = 0;
    virtual void OnStop(RuntimeContext& context) = 0;

private:
    std::string name_;
    ApplicationState state_{ApplicationState::kCreated};
};

using ApplicationFactory = std::function<std::unique_ptr<Application>()>;

} // namespace ara::exec
