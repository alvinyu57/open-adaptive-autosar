#pragma once

#include <chrono>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <optional>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "ara/core/core_error_domain.h"
#include "ara/core/result.h"
#include "tp_service_manifest.h"

namespace openaa::tire_pressure {

struct TirePressureReading final {
    std::string position;
    int pressure_kpa{0};
};

struct TirePressureSample final {
    std::vector<TirePressureReading> readings;
    std::chrono::system_clock::time_point timestamp;
};

inline std::string ToIso8601(std::chrono::system_clock::time_point timestamp) {
    const auto time = std::chrono::system_clock::to_time_t(timestamp);
    std::tm tm_snapshot{};
#if defined(_WIN32)
    gmtime_s(&tm_snapshot, &time);
#else
    gmtime_r(&time, &tm_snapshot);
#endif
    std::ostringstream stream;
    stream << std::put_time(&tm_snapshot, "%FT%TZ");
    return stream.str();
}

inline std::string SerializeMethodRequest(std::string_view command) {
    return std::string(command);
}

inline std::string SerializeFireAndForgetMessage(std::string_view command,
                                                 std::string_view detail) {
    return std::string(command) + ":" + std::string(detail);
}

inline std::optional<std::pair<std::string, std::string>>
DeserializeFireAndForgetMessage(std::string_view payload) {
    const auto separator = payload.find(':');
    if (separator == std::string_view::npos) {
        return std::nullopt;
    }

    return std::pair<std::string, std::string>{
        std::string(payload.substr(0, separator)),
        std::string(payload.substr(separator + 1)),
    };
}

inline std::string SerializeSample(const TirePressureSample& sample) {
    std::ostringstream stream;
    stream << "{\n"
           << "  \"timestamp\": \"" << ToIso8601(sample.timestamp) << "\",\n"
           << "  \"tires\": [\n";

    for (std::size_t index = 0; index < sample.readings.size(); ++index) {
        const auto& reading = sample.readings[index];
        stream << "    {\"position\": \"" << reading.position
               << "\", \"pressureKPa\": " << reading.pressure_kpa << "}";
        if (index + 1U != sample.readings.size()) {
            stream << ',';
        }
        stream << '\n';
    }

    stream << "  ]\n"
           << "}\n";
    return stream.str();
}

inline std::optional<TirePressureSample> DeserializeSample(std::string_view text) {
    static const std::regex tire_pattern(
        R"re(\{\s*"position"\s*:\s*"([^"]+)"\s*,\s*"pressureKPa"\s*:\s*([0-9]+)\s*\})re");

    const std::string owned_text(text);
    TirePressureSample sample;
    sample.timestamp = std::chrono::system_clock::now();
    for (std::sregex_iterator iter(owned_text.begin(), owned_text.end(), tire_pattern), end;
         iter != end;
         ++iter) {
        sample.readings.push_back(TirePressureReading{
            (*iter)[1].str(),
            std::stoi((*iter)[2].str()),
        });
    }

    if (sample.readings.empty()) {
        return std::nullopt;
    }

    return sample;
}

inline ara::core::Result<TirePressureSample>
LoadSampleFromJsonFile(const std::filesystem::path& file_path) noexcept {
    std::ifstream input(file_path);
    if (!input.is_open()) {
        return ara::core::Result<TirePressureSample>{
            ara::core::MakeErrorCode(ara::core::CoreErrc::kNoSuchElement)};
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    auto sample = DeserializeSample(buffer.str());
    if (!sample.has_value()) {
        return ara::core::Result<TirePressureSample>{
            ara::core::MakeErrorCode(ara::core::CoreErrc::kInvalidArgument)};
    }

    return ara::core::Result<TirePressureSample>{std::move(sample.value())};
}

} // namespace openaa::tire_pressure
