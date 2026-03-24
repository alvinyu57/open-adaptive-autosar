#include "openaa/core/application.hpp"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace openaa::core {

namespace {

std::string TimestampNow() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t time_now = std::chrono::system_clock::to_time_t(now);
    std::tm tm_snapshot{};
#if defined(_WIN32)
    localtime_s(&tm_snapshot, &time_now);
#else
    localtime_r(&time_now, &tm_snapshot);
#endif

    std::ostringstream buffer;
    buffer << std::put_time(&tm_snapshot, "%F %T");
    return buffer.str();
}

} // namespace

Logger::Logger(std::ostream &output) : output_(&output) {}

void Logger::Info(std::string_view component, std::string_view message) const {
    (*output_) << "[" << TimestampNow() << "] [INFO] [" << component << "] " << message << '\n';
}

void Logger::Warn(std::string_view component, std::string_view message) const {
    (*output_) << "[" << TimestampNow() << "] [WARN] [" << component << "] " << message << '\n';
}

bool ServiceRegistry::Register(ServiceEntry entry) {
    std::scoped_lock lock(mutex_);
    const auto [it, inserted] = services_.emplace(entry.service_id, std::move(entry));
    return inserted;
}

std::vector<ServiceRegistry::ServiceEntry> ServiceRegistry::List() const {
    std::scoped_lock lock(mutex_);

    std::vector<ServiceEntry> entries;
    entries.reserve(services_.size());

    for (const auto &[_, entry] : services_) {
        entries.push_back(entry);
    }

    return entries;
}

RuntimeContext::RuntimeContext(ServiceRegistry &registry, Logger &logger)
    : registry_(&registry), logger_(&logger) {}

ServiceRegistry &RuntimeContext::Services() const {
    return *registry_;
}

Logger &RuntimeContext::Log() const {
    return *logger_;
}

Application::Application(std::string name) : name_(std::move(name)) {}

void Application::Initialize(RuntimeContext &context) {
    if (state_ != ApplicationState::kCreated) {
        throw std::logic_error("Application can only be initialized once");
    }

    OnInitialize(context);
    state_ = ApplicationState::kInitialized;
}

void Application::Run(RuntimeContext &context) {
    if (state_ != ApplicationState::kInitialized) {
        throw std::logic_error("Application must be initialized before running");
    }

    OnStart(context);
    state_ = ApplicationState::kRunning;
}

void Application::Stop(RuntimeContext &context) {
    if (state_ == ApplicationState::kStopped || state_ == ApplicationState::kCreated) {
        return;
    }

    OnStop(context);
    state_ = ApplicationState::kStopped;
}

const std::string &Application::Name() const {
    return name_;
}

ApplicationState Application::State() const {
    return state_;
}

} // namespace openaa::core
