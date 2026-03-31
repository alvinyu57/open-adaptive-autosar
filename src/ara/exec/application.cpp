#include "ara/exec/application.hpp"

#include <stdexcept>
#include <utility>

namespace ara::exec {

bool ServiceRegistry::Register(ServiceEntry entry) {
    std::scoped_lock lock(mutex_);
    const auto [it, inserted] = services_.emplace(entry.service_id, std::move(entry));
    return inserted;
}

std::vector<ServiceEntry> ServiceRegistry::List() const {
    std::scoped_lock lock(mutex_);

    std::vector<ServiceEntry> entries;
    entries.reserve(services_.size());

    for (const auto& [_, entry] : services_) {
        entries.push_back(entry);
    }

    return entries;
}

Application::Application(std::string name)
    : name_(std::move(name)) {}

void Application::Initialize(RuntimeContext& context) {
    if (state_ != ApplicationState::kCreated) {
        throw std::logic_error("Application can only be initialized once");
    }

    OnInitialize(context);
    state_ = ApplicationState::kInitialized;
}

void Application::Run(RuntimeContext& context) {
    if (state_ != ApplicationState::kInitialized) {
        throw std::logic_error("Application must be initialized before running");
    }

    OnStart(context);
    state_ = ApplicationState::kRunning;
}

void Application::Stop(RuntimeContext& context) {
    if (state_ == ApplicationState::kStopped || state_ == ApplicationState::kCreated) {
        return;
    }

    OnStop(context);
    state_ = ApplicationState::kStopped;
}

const std::string& Application::Name() const {
    return name_;
}

ApplicationState Application::State() const {
    return state_;
}

} // namespace ara::exec
