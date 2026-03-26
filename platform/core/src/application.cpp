#include "openaa/platform/core/application.hpp"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace openaa::platform::core {

namespace {

std::string TimestampNow() {
    const auto now = std::chrono::system_clock::now();
    const auto seconds = std::chrono::time_point_cast<std::chrono::seconds>(now);
    const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now - seconds).count();

    const std::time_t time_now = std::chrono::system_clock::to_time_t(now);
    std::tm tm_snapshot{};

#if defined(_WIN32)
    localtime_s(&tm_snapshot, &time_now);
#else
    localtime_r(&time_now, &tm_snapshot);
#endif

    std::ostringstream buffer;
    
    buffer << std::put_time(&tm_snapshot, "%F %T") << '.'
           << std::setw(3) << std::setfill('0') << milliseconds;

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

void Logger::Error(std::string_view component, std::string_view message) const {
    (*output_) << "[" << TimestampNow() << "] [ERROR] [" << component << "] " << message << '\n';
}

bool ServiceRegistry::Register(ara::core::ServiceEntry entry) {
    std::scoped_lock lock(mutex_);
    const auto [it, inserted] = services_.emplace(entry.service_id, std::move(entry));
    return inserted;
}

std::vector<ara::core::ServiceEntry> ServiceRegistry::List() const {
    std::scoped_lock lock(mutex_);

    std::vector<ara::core::ServiceEntry> entries;
    entries.reserve(services_.size());

    for (const auto &[_, entry] : services_) {
        entries.push_back(entry);
    }

    return entries;
}

Application::Application(std::string name) : name_(std::move(name)) {}

void Application::Initialize(ara::core::RuntimeContext &context) {
    if (state_ != ara::core::ApplicationState::kCreated) {
        throw std::logic_error("Application can only be initialized once");
    }

    OnInitialize(context);
    state_ = ara::core::ApplicationState::kInitialized;
}

void Application::Run(ara::core::RuntimeContext &context) {
    if (state_ != ara::core::ApplicationState::kInitialized) {
        throw std::logic_error("Application must be initialized before running");
    }

    OnStart(context);
    state_ = ara::core::ApplicationState::kRunning;
}

void Application::Stop(ara::core::RuntimeContext &context) {
    if (state_ == ara::core::ApplicationState::kStopped ||
        state_ == ara::core::ApplicationState::kCreated) {
        return;
    }

    OnStop(context);
    state_ = ara::core::ApplicationState::kStopped;
}

const std::string &Application::Name() const {
    return name_;
}

ara::core::ApplicationState Application::State() const {
    return state_;
}

} // namespace openaa::platform::core
