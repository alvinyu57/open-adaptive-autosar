#include "com_runtime_state.hpp"

#include <cctype>
#include <filesystem>
#include <fstream>
#include <limits>
#include <optional>
#include <sstream>

namespace ara::com::runtime::internal {

namespace {

std::filesystem::path RegistryRoot() {
    return std::filesystem::temp_directory_path() / "openaa_ara_com" / "registry";
}

std::string Sanitize(std::string_view value) {
    std::string sanitized;
    sanitized.reserve(value.size());
    for (const char character : value) {
        if (std::isalnum(static_cast<unsigned char>(character)) != 0) {
            sanitized.push_back(character);
        } else {
            sanitized.push_back('_');
        }
    }
    return sanitized;
}

std::filesystem::path InstanceMappingPath(std::string_view instance_specifier) {
    return RegistryRoot() / "instances" / (Sanitize(instance_specifier) + ".map");
}

std::filesystem::path ServiceRecordPath(std::string_view service_id,
                                        std::string_view instance_identifier) {
    return RegistryRoot() / "services" /
           (Sanitize(service_id) + "__" + Sanitize(instance_identifier) + ".svc");
}

bool EnsureDirectory(const std::filesystem::path& path) {
    std::error_code error_code;
    std::filesystem::create_directories(path, error_code);
    return !error_code;
}

bool WriteTextFile(const std::filesystem::path& path, std::string_view content) {
    if (!EnsureDirectory(path.parent_path())) {
        return false;
    }

    const auto temp_path = path.string() + ".tmp";
    {
        std::ofstream output(temp_path, std::ios::trunc);
        if (!output.is_open()) {
            return false;
        }
        output << content;
    }

    std::error_code error_code;
    std::filesystem::rename(temp_path, path, error_code);
    return !error_code;
}

std::optional<std::string> ReadTextFile(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input.is_open()) {
        return std::nullopt;
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

std::string SerializeBindingMetadata(const BindingMetadata& metadata) {
    std::ostringstream stream;
    stream << static_cast<int>(metadata.binding_type) << '\n' << metadata.endpoint.View() << '\n';
    return stream.str();
}

std::optional<BindingMetadata> DeserializeBindingMetadata(std::string_view text) {
    std::istringstream stream{std::string(text)};
    int binding_type_value = 0;
    std::string endpoint;
    if (!(stream >> binding_type_value)) {
        return std::nullopt;
    }
    stream.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    if (!std::getline(stream, endpoint)) {
        return std::nullopt;
    }

    return BindingMetadata{
        static_cast<BindingType>(binding_type_value),
        ara::core::String(endpoint),
    };
}

class IpcBindingRuntime final : public IBindingRuntime {
public:
    ara::core::Result<void> OfferService(const ServiceRecord& record) noexcept override {
        std::lock_guard<std::mutex> lock(mutex_);

        const ServiceKey key{record.service_id, record.instance_identifier};
        if (services_.find(key) != services_.end()) {
            return ara::core::Result<void>{MakeErrorCode(ComErrc::kServiceNotAvailable)};
        }

        services_.emplace(key, record.metadata);
        if (!WriteTextFile(ServiceRecordPath(record.service_id.toString(),
                                             record.instance_identifier.toString()),
                           SerializeBindingMetadata(record.metadata))) {
            return ara::core::Result<void>{MakeErrorCode(ComErrc::kCommunicationFailure)};
        }

        return ara::core::Result<void>{};
    }

    ara::core::Result<void>
    StopOfferService(const ara::com::ServiceIdentifierType& service_id,
                     const ara::com::InstanceIdentifier& instance_identifier) noexcept override {
        std::lock_guard<std::mutex> lock(mutex_);

        const ServiceKey key{service_id, instance_identifier};
        if (services_.erase(key) == 0U) {
            const auto path =
                ServiceRecordPath(service_id.toString(), instance_identifier.toString());
            std::error_code error_code;
            if (!std::filesystem::remove(path, error_code)) {
                return ara::core::Result<void>{MakeErrorCode(ComErrc::kServiceNotOffered)};
            }
            return ara::core::Result<void>{};
        }

        std::error_code error_code;
        std::filesystem::remove(
            ServiceRecordPath(service_id.toString(), instance_identifier.toString()), error_code);
        return ara::core::Result<void>{};
    }

    ara::core::Result<ara::core::Vector<BindingMetadata>>
    FindServices(const ara::com::ServiceIdentifierType& service_id,
                 const ara::com::InstanceIdentifier& instance_identifier) noexcept override {
        std::lock_guard<std::mutex> lock(mutex_);

        ara::core::Vector<BindingMetadata> matches;
        const ServiceKey key{service_id, instance_identifier};
        const auto iter = services_.find(key);
        if (iter != services_.end()) {
            matches.push_back(iter->second);
            return ara::core::Result<ara::core::Vector<BindingMetadata>>{std::move(matches)};
        }

        const auto content =
            ReadTextFile(ServiceRecordPath(service_id.toString(), instance_identifier.toString()));
        if (content.has_value()) {
            auto metadata = DeserializeBindingMetadata(*content);
            if (metadata.has_value()) {
                matches.push_back(*metadata);
            }
        }

        return ara::core::Result<ara::core::Vector<BindingMetadata>>{std::move(matches)};
    }

private:
    using ServiceKey = ComRuntimeState::ServiceKey;

    std::mutex mutex_;
    std::map<ServiceKey, BindingMetadata> services_;
};

} // namespace

ComRuntimeState& ComRuntimeState::Instance() noexcept {
    static ComRuntimeState instance;
    return instance;
}

ara::core::Result<void>
ComRuntimeState::RegisterInstanceMapping(const ara::core::InstanceSpecifier& instance_specifier,
                                         const ara::com::InstanceIdentifier& instance_identifier,
                                         const BindingMetadata& metadata) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);

    auto& mappings = instance_mappings_[std::string(instance_specifier.View())];
    for (const auto& mapping : mappings) {
        if (mapping.instance_identifier == instance_identifier) {
            return ara::core::Result<void>{};
        }
    }

    mappings.push_back(InstanceMapping{instance_identifier, metadata});
    std::ostringstream serialized_mapping;
    serialized_mapping << instance_identifier.toString() << '\n'
                       << SerializeBindingMetadata(metadata);
    if (!WriteTextFile(InstanceMappingPath(instance_specifier.View()), serialized_mapping.str())) {
        return ara::core::Result<void>{MakeErrorCode(ComErrc::kCommunicationFailure)};
    }

    return ara::core::Result<void>{};
}

ara::core::Result<ara::com::InstanceIdentifierContainer> ComRuntimeState::ResolveInstanceIDs(
    const ara::core::InstanceSpecifier& instance_specifier) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);

    const auto iter = instance_mappings_.find(std::string(instance_specifier.View()));
    ara::com::InstanceIdentifierContainer result;
    if (iter != instance_mappings_.end()) {
        result.reserve(iter->second.size());
        for (const auto& mapping : iter->second) {
            result.push_back(mapping.instance_identifier);
        }
    }

    if (result.empty()) {
        const auto content = ReadTextFile(InstanceMappingPath(instance_specifier.View()));
        if (content.has_value()) {
            std::istringstream stream(*content);
            std::string instance_id;
            std::getline(stream, instance_id);
            if (!instance_id.empty()) {
                result.push_back(ara::com::InstanceIdentifier::Create(instance_id));
            }
        }
    }

    if (result.empty()) {
        return ara::core::Result<ara::com::InstanceIdentifierContainer>{
            MakeErrorCode(ComErrc::kInstanceIDNotResolvable)};
    }

    return ara::core::Result<ara::com::InstanceIdentifierContainer>{std::move(result)};
}

ara::core::Result<void>
ComRuntimeState::OfferService(const ara::com::ServiceIdentifierType& service_id,
                              const ara::com::InstanceIdentifier& instance_identifier,
                              const BindingMetadata& metadata) noexcept {
    ServiceRecord record{
        service_id,
        instance_identifier,
        metadata,
    };

    return GetOrCreateBindingRuntime(metadata.binding_type).OfferService(record);
}

ara::core::Result<void> ComRuntimeState::StopOfferService(
    const ara::com::ServiceIdentifierType& service_id,
    const ara::com::InstanceIdentifier& instance_identifier) noexcept {
    auto& ipc_runtime = GetOrCreateBindingRuntime(BindingType::kIpc);
    auto result = ipc_runtime.StopOfferService(service_id, instance_identifier);
    if (result.HasValue()) {
        return result;
    }

    return ara::core::Result<void>{MakeErrorCode(ComErrc::kServiceNotOffered)};
}

ara::core::Result<ara::core::Vector<BindingMetadata>>
ComRuntimeState::FindServices(const ara::com::ServiceIdentifierType& service_id,
                              const ara::com::InstanceIdentifier& instance_identifier) noexcept {
    ara::core::Vector<BindingMetadata> matches;
    auto& ipc_runtime = GetOrCreateBindingRuntime(BindingType::kIpc);
    auto result = ipc_runtime.FindServices(service_id, instance_identifier);
    if (!result.HasValue()) {
        return result;
    }

    for (const auto& metadata : result.Value()) {
        matches.push_back(metadata);
    }

    return ara::core::Result<ara::core::Vector<BindingMetadata>>{std::move(matches)};
}

IBindingRuntime& ComRuntimeState::GetOrCreateBindingRuntime(BindingType binding_type) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto iter = binding_runtimes_.find(binding_type);
    if (iter == binding_runtimes_.end()) {
        iter = binding_runtimes_.emplace(binding_type, std::make_unique<IpcBindingRuntime>()).first;
    }

    return *iter->second;
}

} // namespace ara::com::runtime::internal
