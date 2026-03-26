#pragma once

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace ara::core {

enum class ApplicationState { kCreated, kInitialized, kRunning, kStopped };

struct ServiceEntry {
    std::string service_id;
    std::string endpoint;
    std::string owner;
};

class Logger {
  public:
    virtual ~Logger() = default;

    virtual void Info(std::string_view component, std::string_view message) const = 0;
    virtual void Warn(std::string_view component, std::string_view message) const = 0;
    virtual void Error(std::string_view component, std::string_view message) const = 0;
};

class ServiceRegistry {
  public:
    virtual ~ServiceRegistry() = default;

    virtual bool Register(ServiceEntry entry) = 0;
    virtual std::vector<ServiceEntry> List() const = 0;
};

class RuntimeContext {
  public:
    RuntimeContext(ServiceRegistry &registry, Logger &logger) : registry_(&registry), logger_(&logger) {}

    ServiceRegistry &Services() const {
        return *registry_;
    }

    Logger &Log() const {
        return *logger_;
    }

  private:
    ServiceRegistry *registry_;
    Logger *logger_;
};

class Application {
  public:
    virtual ~Application() = default;

    virtual void Initialize(RuntimeContext &context) = 0;
    virtual void Run(RuntimeContext &context) = 0;
    virtual void Stop(RuntimeContext &context) = 0;

    virtual const std::string &Name() const = 0;
    virtual ApplicationState State() const = 0;
};

using ApplicationFactory = std::function<std::unique_ptr<Application>()>;

} // namespace ara::core
