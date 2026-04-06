#pragma once

#include <ostream>
#include <string_view>

namespace ara::log {

class Logger {
public:
    explicit Logger(std::ostream& output);
    virtual ~Logger() = default;

    virtual void Info(std::string_view component, std::string_view message) const;
    virtual void Warn(std::string_view component, std::string_view message) const;
    virtual void Error(std::string_view component, std::string_view message) const;

private:
    std::ostream* output_;
};

} // namespace ara::log
