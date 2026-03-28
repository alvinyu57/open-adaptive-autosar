#include "ara/exec/application_manifest.hpp"

#include <cctype>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace openaa::exec {
namespace {

class JsonParser {
public:
    explicit JsonParser(std::string document)
        : document_(std::move(document)) {}

    struct JsonObject;
    struct JsonArray;

    struct JsonValue {
        std::variant<
            bool,
            std::string,
            std::shared_ptr<JsonObject>,
            std::shared_ptr<JsonArray>>
            storage;
    };

    struct JsonObject {
        std::map<std::string, JsonValue> values;
    };

    struct JsonArray {
        std::vector<JsonValue> values;
    };

    JsonValue Parse() {
        SkipWhitespace();
        JsonValue value = ParseValue();
        SkipWhitespace();

        if (!AtEnd()) {
            throw std::runtime_error("unexpected trailing content in manifest");
        }

        return value;
    }

private:
    JsonValue ParseValue() {
        SkipWhitespace();

        if (AtEnd()) {
            throw std::runtime_error("unexpected end of manifest");
        }

        const char current = document_[position_];
        if (current == '"') {
            return JsonValue{std::variant<
                bool,
                std::string,
                std::shared_ptr<JsonObject>,
                std::shared_ptr<JsonArray>>(ParseString())};
        }
        if (current == '{') {
            return JsonValue{std::variant<
                bool,
                std::string,
                std::shared_ptr<JsonObject>,
                std::shared_ptr<JsonArray>>(ParseObject())};
        }
        if (current == '[') {
            return JsonValue{std::variant<
                bool,
                std::string,
                std::shared_ptr<JsonObject>,
                std::shared_ptr<JsonArray>>(ParseArray())};
        }
        if (ConsumeLiteral("true")) {
            return JsonValue{std::variant<
                bool,
                std::string,
                std::shared_ptr<JsonObject>,
                std::shared_ptr<JsonArray>>(true)};
        }
        if (ConsumeLiteral("false")) {
            return JsonValue{std::variant<
                bool,
                std::string,
                std::shared_ptr<JsonObject>,
                std::shared_ptr<JsonArray>>(false)};
        }

        throw std::runtime_error("unsupported JSON value in manifest");
    }

    std::shared_ptr<JsonObject> ParseObject() {
        Expect('{');
        SkipWhitespace();

        auto object = std::make_shared<JsonObject>();
        if (TryConsume('}')) {
            return object;
        }

        while (true) {
            SkipWhitespace();
            const std::string key = ParseString();
            SkipWhitespace();
            Expect(':');
            SkipWhitespace();
            object->values.emplace(key, ParseValue());
            SkipWhitespace();

            if (TryConsume('}')) {
                return object;
            }

            Expect(',');
        }
    }

    std::shared_ptr<JsonArray> ParseArray() {
        Expect('[');
        SkipWhitespace();

        auto array = std::make_shared<JsonArray>();
        if (TryConsume(']')) {
            return array;
        }

        while (true) {
            array->values.push_back(ParseValue());
            SkipWhitespace();

            if (TryConsume(']')) {
                return array;
            }

            Expect(',');
        }
    }

    std::string ParseString() {
        Expect('"');

        std::string value;
        while (!AtEnd()) {
            const char current = document_[position_++];
            if (current == '"') {
                return value;
            }

            if (current != '\\') {
                value.push_back(current);
                continue;
            }

            if (AtEnd()) {
                throw std::runtime_error("unterminated escape sequence in manifest");
            }

            const char escaped = document_[position_++];
            switch (escaped) {
            case '"':
            case '\\':
            case '/':
                value.push_back(escaped);
                break;
            case 'b':
                value.push_back('\b');
                break;
            case 'f':
                value.push_back('\f');
                break;
            case 'n':
                value.push_back('\n');
                break;
            case 'r':
                value.push_back('\r');
                break;
            case 't':
                value.push_back('\t');
                break;
            default:
                throw std::runtime_error("unsupported escape sequence in manifest");
            }
        }

        throw std::runtime_error("unterminated string in manifest");
    }

    void SkipWhitespace() {
        while (!AtEnd() &&
               std::isspace(static_cast<unsigned char>(document_[position_])) != 0) {
            ++position_;
        }
    }

    void Expect(char expected) {
        if (AtEnd() || document_[position_] != expected) {
            throw std::runtime_error("unexpected JSON token in manifest");
        }

        ++position_;
    }

    bool TryConsume(char expected) {
        if (AtEnd() || document_[position_] != expected) {
            return false;
        }

        ++position_;
        return true;
    }

    bool ConsumeLiteral(std::string_view literal) {
        if (document_.compare(position_, literal.size(), literal) != 0) {
            return false;
        }

        position_ += literal.size();
        return true;
    }

    bool AtEnd() const {
        return position_ >= document_.size();
    }

    std::string document_;
    std::size_t position_{0U};
};

const JsonParser::JsonObject& AsObject(
    const JsonParser::JsonValue& value,
    std::string_view field_name) {
    const auto* object = std::get_if<std::shared_ptr<JsonParser::JsonObject>>(&value.storage);
    if (object == nullptr || *object == nullptr) {
        throw std::runtime_error("manifest field '" + std::string(field_name) + "' must be an object");
    }

    return **object;
}

const JsonParser::JsonArray& AsArray(
    const JsonParser::JsonValue& value,
    std::string_view field_name) {
    const auto* array = std::get_if<std::shared_ptr<JsonParser::JsonArray>>(&value.storage);
    if (array == nullptr || *array == nullptr) {
        throw std::runtime_error("manifest field '" + std::string(field_name) + "' must be an array");
    }

    return **array;
}

const std::string& AsString(
    const JsonParser::JsonValue& value,
    std::string_view field_name) {
    const auto* string = std::get_if<std::string>(&value.storage);
    if (string == nullptr) {
        throw std::runtime_error("manifest field '" + std::string(field_name) + "' must be a string");
    }

    return *string;
}

bool AsBool(const JsonParser::JsonValue& value, std::string_view field_name) {
    const auto* boolean = std::get_if<bool>(&value.storage);
    if (boolean == nullptr) {
        throw std::runtime_error("manifest field '" + std::string(field_name) + "' must be a boolean");
    }

    return *boolean;
}

const JsonParser::JsonValue& RequireField(
    const JsonParser::JsonObject& object,
    std::string_view field_name) {
    const auto it = object.values.find(std::string(field_name));
    if (it == object.values.end()) {
        throw std::runtime_error("missing required manifest field '" + std::string(field_name) + "'");
    }

    return it->second;
}

const JsonParser::JsonValue* FindField(
    const JsonParser::JsonObject& object,
    std::string_view field_name) {
    const auto it = object.values.find(std::string(field_name));
    if (it == object.values.end()) {
        return nullptr;
    }

    return &it->second;
}

std::vector<std::string> ReadArguments(const JsonParser::JsonObject& object) {
    const JsonParser::JsonValue* value = FindField(object, "arguments");
    if (value == nullptr) {
        return {};
    }

    std::vector<std::string> arguments;
    for (const auto& entry : AsArray(*value, "arguments").values) {
        arguments.push_back(AsString(entry, "arguments"));
    }

    return arguments;
}

std::map<std::string, std::string> ReadEnvironmentVariables(
    const JsonParser::JsonObject& object) {
    const JsonParser::JsonValue* value = FindField(object, "environmentVariables");
    if (value == nullptr) {
        return {};
    }

    std::map<std::string, std::string> environment_variables;
    for (const auto& [key, entry] : AsObject(*value, "environmentVariables").values) {
        environment_variables.emplace(key, AsString(entry, "environmentVariables"));
    }

    return environment_variables;
}

std::vector<ara::exec::ProvidedServiceManifest> ReadProvidedServices(
    const JsonParser::JsonObject& object) {
    const JsonParser::JsonValue* value = FindField(object, "providedServices");
    if (value == nullptr) {
        return {};
    }

    std::vector<ara::exec::ProvidedServiceManifest> services;
    for (const auto& entry : AsArray(*value, "providedServices").values) {
        const auto& service_object = AsObject(entry, "providedServices");
        services.push_back({
            .service_id = AsString(RequireField(service_object, "serviceId"), "serviceId"),
            .endpoint = AsString(RequireField(service_object, "endpoint"), "endpoint"),
        });
    }

    return services;
}

void ValidateManifest(const ara::exec::ApplicationManifest& manifest) {
    if (manifest.application_id.empty()) {
        throw std::runtime_error("manifest field 'applicationId' must not be empty");
    }

    if (manifest.short_name.empty()) {
        throw std::runtime_error("manifest field 'shortName' must not be empty");
    }

    if (manifest.executable.empty()) {
        throw std::runtime_error("manifest field 'executable' must not be empty");
    }
}

} // namespace

ara::exec::ApplicationManifest LoadApplicationManifestFromFile(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input.is_open()) {
        throw std::runtime_error("unable to open application manifest: " + path.string());
    }

    std::string document{
        std::istreambuf_iterator<char>(input),
        std::istreambuf_iterator<char>()};

    const auto root = JsonParser(std::move(document)).Parse();
    const auto& object = AsObject(root, "root");

    ara::exec::ApplicationManifest manifest{
        .application_id = AsString(RequireField(object, "applicationId"), "applicationId"),
        .short_name = AsString(RequireField(object, "shortName"), "shortName"),
        .executable = AsString(RequireField(object, "executable"), "executable"),
        .arguments = ReadArguments(object),
        .environment_variables = ReadEnvironmentVariables(object),
        .provided_services = ReadProvidedServices(object),
        .auto_start = true,
    };

    if (const JsonParser::JsonValue* value = FindField(object, "autoStart"); value != nullptr) {
        manifest.auto_start = AsBool(*value, "autoStart");
    }

    ValidateManifest(manifest);
    return manifest;
}

} // namespace openaa::exec
