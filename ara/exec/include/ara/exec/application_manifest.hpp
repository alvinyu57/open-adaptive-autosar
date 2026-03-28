#pragma once

#include <map>
#include <string>
#include <vector>

namespace ara::exec {

struct ProvidedServiceManifest {
    std::string service_id;
    std::string endpoint;
};

struct ApplicationManifest {
    std::string application_id;
    std::string short_name;
    std::string executable;
    std::vector<std::string> arguments;
    std::map<std::string, std::string> environment_variables;
    std::vector<ProvidedServiceManifest> provided_services;
    bool auto_start{true};
};

} // namespace ara::exec
