#pragma once

#include "ara/log/logger.hpp"

#include <map>
#include <mutex>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace ara::core {

enum class ApplicationState : std::uint8_t {
    kCreated,
    kInitialized,
    kRunning,
    kStopped
};

struct ServiceEntry {
    std::string service_id;
    std::string endpoint;
    std::string owner;
};

class ServiceRegistry {
public:
    virtual ~ServiceRegistry() = default;

    virtual bool Register(ServiceEntry entry);
    virtual std::vector<ServiceEntry> List() const;

private:
    mutable std::mutex mutex_;
    std::map<std::string, ServiceEntry> services_;
};

class RuntimeContext {
public:
    RuntimeContext(ServiceRegistry& registry, ara::log::Logger& logger)
        : registry_(&registry),
          logger_(&logger) {}

    ServiceRegistry& Services() const {
        return *registry_;
    }

    ara::log::Logger& Log() const {
        return *logger_;
    }

private:
    ServiceRegistry* registry_;
    ara::log::Logger* logger_;
};

class Application {
public:
    explicit Application(std::string name);
    virtual ~Application() = default;

    virtual void Initialize(RuntimeContext& context);
    virtual void Run(RuntimeContext& context);
    virtual void Stop(RuntimeContext& context);

    virtual const std::string& Name() const;
    virtual ApplicationState State() const;

protected:
    virtual void OnInitialize(RuntimeContext& context) = 0;
    virtual void OnStart(RuntimeContext& context) = 0;
    virtual void OnStop(RuntimeContext& context) = 0;

private:
    std::string name_;
    ApplicationState state_{ApplicationState::kCreated};
};

using ApplicationFactory = std::function<std::unique_ptr<Application>()>;

} // namespace ara::core
