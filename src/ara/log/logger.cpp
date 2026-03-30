#include "ara/log/logger.hpp"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace ara::log {

namespace {

std::string TimestampNow() {
    const auto now = std::chrono::system_clock::now();
    const auto seconds = std::chrono::time_point_cast<std::chrono::seconds>(now);
    const auto milliseconds =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - seconds).count();

    const std::time_t time_now = std::chrono::system_clock::to_time_t(now);
    std::tm tm_snapshot{};

#if defined(_WIN32)
    localtime_s(&tm_snapshot, &time_now);
#else
    localtime_r(&time_now, &tm_snapshot);
#endif

    std::ostringstream buffer;
    buffer << std::put_time(&tm_snapshot, "%F %T") << '.' << std::setw(3) << std::setfill('0')
           << milliseconds;
    return buffer.str();
}

} // namespace

Logger::Logger(std::ostream& output)
    : output_(&output) {}

void Logger::Info(std::string_view component, std::string_view message) const {
    (*output_) << "[" << TimestampNow() << "] [INFO] [" << component << "] " << message << '\n';
}

void Logger::Warn(std::string_view component, std::string_view message) const {
    (*output_) << "[" << TimestampNow() << "] [WARN] [" << component << "] " << message << '\n';
}

void Logger::Error(std::string_view component, std::string_view message) const {
    (*output_) << "[" << TimestampNow() << "] [ERROR] [" << component << "] " << message << '\n';
}

} // namespace ara::log
