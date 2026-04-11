#pragma once

#include <fstream>
#include <optional>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>

#include "ara/com/service/instance_identifier.h"
#include "ara/com/service/service_identifier.h"
#include "ara/core/core_error_domain.h"
#include "ara/core/instance_specifier.h"
#include "ara/core/result.h"

namespace openaa::tire_pressure {

struct TirePressureServiceManifest final {
    ara::core::InstanceSpecifier instance_specifier;
    ara::com::InstanceIdentifier instance_identifier;
    ara::com::ServiceIdentifierType service_identifier;
    std::string event_channel;
    std::string method_channel;
    std::string fire_and_forget_channel;
};

inline ara::core::Result<TirePressureServiceManifest> LoadServiceManifest(
    const std::string& manifest_path) noexcept {
    std::ifstream input(manifest_path);
    if (!input.is_open()) {
        return ara::core::Result<TirePressureServiceManifest>{
            ara::core::MakeErrorCode(ara::core::CoreErrc::kNoSuchElement)};
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    const std::string json = buffer.str();

    const std::regex instance_specifier_pattern(
        R"re("instanceSpecifier"\s*:\s*"([^"]+)")re");
    const std::regex instance_identifier_pattern(
        R"re("instanceIdentifier"\s*:\s*"([^"]+)")re");
    const std::regex service_identifier_pattern(
        R"re("serviceIdentifier"\s*:\s*"([^"]+)")re");
    const std::regex event_channel_pattern(
        R"re("eventChannel"\s*:\s*"([^"]+)")re");
    const std::regex method_channel_pattern(
        R"re("methodChannel"\s*:\s*"([^"]+)")re");
    const std::regex fire_and_forget_channel_pattern(
        R"re("fireAndForgetChannel"\s*:\s*"([^"]+)")re");

    std::smatch match;
    if (!std::regex_search(json, match, instance_specifier_pattern)) {
        return ara::core::Result<TirePressureServiceManifest>{
            ara::core::MakeErrorCode(ara::core::CoreErrc::kInvalidArgument)};
    }
    const std::string instance_specifier = match[1].str();

    if (!std::regex_search(json, match, instance_identifier_pattern)) {
        return ara::core::Result<TirePressureServiceManifest>{
            ara::core::MakeErrorCode(ara::core::CoreErrc::kInvalidArgument)};
    }
    const std::string instance_identifier = match[1].str();

    if (!std::regex_search(json, match, service_identifier_pattern)) {
        return ara::core::Result<TirePressureServiceManifest>{
            ara::core::MakeErrorCode(ara::core::CoreErrc::kInvalidArgument)};
    }
    const std::string service_identifier = match[1].str();

    if (!std::regex_search(json, match, event_channel_pattern)) {
        return ara::core::Result<TirePressureServiceManifest>{
            ara::core::MakeErrorCode(ara::core::CoreErrc::kInvalidArgument)};
    }
    const std::string event_channel = match[1].str();

    if (!std::regex_search(json, match, method_channel_pattern)) {
        return ara::core::Result<TirePressureServiceManifest>{
            ara::core::MakeErrorCode(ara::core::CoreErrc::kInvalidArgument)};
    }
    const std::string method_channel = match[1].str();

    if (!std::regex_search(json, match, fire_and_forget_channel_pattern)) {
        return ara::core::Result<TirePressureServiceManifest>{
            ara::core::MakeErrorCode(ara::core::CoreErrc::kInvalidArgument)};
    }
    const std::string fire_and_forget_channel = match[1].str();

    return ara::core::Result<TirePressureServiceManifest>{TirePressureServiceManifest{
        ara::core::InstanceSpecifier(instance_specifier),
        ara::com::InstanceIdentifier::Create(instance_identifier),
        ara::com::ServiceIdentifierType(service_identifier),
        event_channel,
        method_channel,
        fire_and_forget_channel,
    }};
}

} // namespace openaa::tire_pressure
