#pragma once

#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace openaa::core {

enum class ApplicationState {
    kCreated,
    kInitialized,
    kRunning,
    kStopped
};

class Logger {
public:
    explicit Logger(std::ostream& output);

    void Info(std::string_view component, std::string_view message) const;
    void Warn(std::string_view component, std::string_view message) const;

private:
    std::ostream* output_;
};

class ServiceRegistry {
public:
    struct ServiceEntry {
        std::string service_id;
        std::string endpoint;
        std::string owner;
    };

    bool Register(ServiceEntry entry);
    std::vector<ServiceEntry> List() const;

private:
    mutable std::mutex mutex_;
    std::map<std::string, ServiceEntry> services_;
};

class RuntimeContext {
public:
    RuntimeContext(ServiceRegistry& registry, Logger& logger);

    ServiceRegistry& Services() const;
    Logger& Log() const;

private:
    ServiceRegistry* registry_;
    Logger* logger_;
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

}  // namespace openaa::core
