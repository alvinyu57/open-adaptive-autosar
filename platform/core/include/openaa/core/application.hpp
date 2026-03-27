#pragma once

#include <map>
#include <mutex>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

#include "ara/core/application.hpp"

namespace openaa::core {

class Logger final : public ara::core::Logger {

public:
    explicit Logger(std::ostream& output);

    void Info(std::string_view component, std::string_view message) const override;
    void Warn(std::string_view component, std::string_view message) const override;
    void Error(std::string_view component, std::string_view message) const override;

private:
    std::ostream* output_;
};

class ServiceRegistry final : public ara::core::ServiceRegistry {
public:
    bool Register(ara::core::ServiceEntry entry) override;
    std::vector<ara::core::ServiceEntry> List() const override;

private:
    mutable std::mutex mutex_;
    std::map<std::string, ara::core::ServiceEntry> services_;
};

class Application : public ara::core::Application {
public:
    explicit Application(std::string name);
    ~Application() override = default;

    void Initialize(ara::core::RuntimeContext& context) override;
    void Run(ara::core::RuntimeContext& context) override;
    void Stop(ara::core::RuntimeContext& context) override;

    const std::string& Name() const override;
    ara::core::ApplicationState State() const override;

protected:
    virtual void OnInitialize(ara::core::RuntimeContext& context) = 0;
    virtual void OnStart(ara::core::RuntimeContext& context) = 0;
    virtual void OnStop(ara::core::RuntimeContext& context) = 0;

private:
    std::string name_;
    ara::core::ApplicationState state_{ara::core::ApplicationState::kCreated};
};

} // namespace openaa::core
