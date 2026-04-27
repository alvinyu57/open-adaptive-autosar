#pragma once

#include <filesystem>
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
    
    std::uint16_t someip_service_id;
    std::uint16_t someip_instance_id;
    std::uint16_t someip_event_id;
    std::uint16_t someip_eventgroup_id;
    std::uint16_t someip_method_id;
    std::uint16_t someip_fire_and_forget_id;
};

/// Resolve the manifest path, falling back to a path relative to the executable
/// when the compile-time absolute path does not exist (e.g. Docker build run on host).
inline std::string ResolveManifestPath(const std::string& manifest_path) {
    if (std::filesystem::exists(manifest_path)) {
        return manifest_path;
    }
    // Fallback: resolve relative to the executable directory via /proc/self/exe.
    // The manifest sits at <exe_dir>/manifests/<filename>.
    try {
        std::filesystem::path exe_dir =
            std::filesystem::read_symlink("/proc/self/exe").parent_path();
        std::filesystem::path filename =
            std::filesystem::path(manifest_path).filename();
        std::filesystem::path candidate = exe_dir / "manifests" / filename;
        if (std::filesystem::exists(candidate)) {
            return candidate.string();
        }
    } catch (const std::exception&) {
        // ignore – will return original path and let the caller report the error
    }
    return manifest_path;
}

inline ara::core::Result<TirePressureServiceManifest>
LoadServiceManifest(const std::string& manifest_path) noexcept {
    const std::string resolved_path = ResolveManifestPath(manifest_path);
    std::ifstream input(resolved_path);
    if (!input.is_open()) {
        return ara::core::Result<TirePressureServiceManifest>{
            ara::core::MakeErrorCode(ara::core::CoreErrc::kNoSuchElement)};
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    const std::string json = buffer.str();

    const std::regex instance_specifier_pattern(R"re("instanceSpecifier"\s*:\s*"([^"]+)")re");
    const std::regex instance_identifier_pattern(R"re("instanceIdentifier"\s*:\s*"([^"]+)")re");
    const std::regex service_identifier_pattern(R"re("serviceIdentifier"\s*:\s*"([^"]+)")re");
    const std::regex event_channel_pattern(R"re("eventChannel"\s*:\s*"([^"]+)")re");
    const std::regex method_channel_pattern(R"re("methodChannel"\s*:\s*"([^"]+)")re");
    const std::regex fire_and_forget_channel_pattern(
        R"re("fireAndForgetChannel"\s*:\s*"([^"]+)")re");
        
    const std::regex someip_service_id_pattern(R"re("someipServiceId"\s*:\s*"([^"]+)")re");
    const std::regex someip_instance_id_pattern(R"re("someipInstanceId"\s*:\s*"([^"]+)")re");
    const std::regex someip_event_id_pattern(R"re("someipEventId"\s*:\s*"([^"]+)")re");
    const std::regex someip_eventgroup_id_pattern(R"re("someipEventGroupId"\s*:\s*"([^"]+)")re");
    const std::regex someip_method_id_pattern(R"re("someipMethodId"\s*:\s*"([^"]+)")re");
    const std::regex someip_ff_id_pattern(R"re("someipFireAndForgetId"\s*:\s*"([^"]+)")re");

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
    
    std::uint16_t service_id = 0, instance_id = 0, event_id = 0, eventgroup_id = 0, method_id = 0, ff_id = 0;
    if (std::regex_search(json, match, someip_service_id_pattern)) service_id = std::stoul(match[1].str(), nullptr, 0);
    if (std::regex_search(json, match, someip_instance_id_pattern)) instance_id = std::stoul(match[1].str(), nullptr, 0);
    if (std::regex_search(json, match, someip_event_id_pattern)) event_id = std::stoul(match[1].str(), nullptr, 0);
    if (std::regex_search(json, match, someip_eventgroup_id_pattern)) eventgroup_id = std::stoul(match[1].str(), nullptr, 0);
    if (std::regex_search(json, match, someip_method_id_pattern)) method_id = std::stoul(match[1].str(), nullptr, 0);
    if (std::regex_search(json, match, someip_ff_id_pattern)) ff_id = std::stoul(match[1].str(), nullptr, 0);
    
    // Construct someip channel names if it's SOMEIP binding
    std::string ec = event_channel;
    std::string mc = method_channel;
    std::string fc = fire_and_forget_channel;
    if (service_id != 0) {
        ec = std::to_string(service_id) + ":" + std::to_string(instance_id) + ":" + std::to_string(event_id) + ":" + std::to_string(eventgroup_id);
        mc = std::to_string(service_id) + ":" + std::to_string(instance_id) + ":" + std::to_string(method_id);
        fc = std::to_string(service_id) + ":" + std::to_string(instance_id) + ":" + std::to_string(ff_id);
    }

    return ara::core::Result<TirePressureServiceManifest>{TirePressureServiceManifest{
        ara::core::InstanceSpecifier(instance_specifier),
        ara::com::InstanceIdentifier::Create(instance_identifier),
        ara::com::ServiceIdentifierType(service_identifier),
        ec,
        mc,
        fc,
        service_id, instance_id, event_id, eventgroup_id, method_id, ff_id
    }};
}

} // namespace openaa::tire_pressure
