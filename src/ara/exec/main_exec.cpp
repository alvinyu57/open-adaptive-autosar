#include <array>
#include <cerrno>
#include <cctype>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <map>
#include <cstring>
#include <poll.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <unistd.h>
#include <utility>
#include <vector>

#include "ara/core/initialization.h"
#include "ara/log/logger.hpp"

extern char** environ;

namespace {

constexpr std::string_view kExecutionReportSocketEnv = "OPENAA_EXEC_REPORT_SOCKET";
constexpr std::string_view kExecutionReportProcessEnv = "OPENAA_EXEC_PROCESS_NAME";
constexpr std::string_view kManagedFunctionGroup = "MachineFG";

class JsonValue final {
public:
    using Object = std::map<std::string, JsonValue>;
    using Array = std::vector<JsonValue>;

    enum class Kind {
        kNull,
        kString,
        kBool,
        kNumber,
        kObject,
        kArray
    };

    JsonValue() = default;

    explicit JsonValue(std::string value)
        : kind_(Kind::kString),
          string_value_(std::move(value)) {}

    explicit JsonValue(bool value)
        : kind_(Kind::kBool),
          bool_value_(value) {}

    explicit JsonValue(double value)
        : kind_(Kind::kNumber),
          number_value_(value) {}

    explicit JsonValue(Object value)
        : kind_(Kind::kObject),
          object_value_(std::move(value)) {}

    explicit JsonValue(Array value)
        : kind_(Kind::kArray),
          array_value_(std::move(value)) {}

    [[nodiscard]] bool IsObject() const noexcept {
        return kind_ == Kind::kObject;
    }

    [[nodiscard]] bool IsArray() const noexcept {
        return kind_ == Kind::kArray;
    }

    [[nodiscard]] bool IsString() const noexcept {
        return kind_ == Kind::kString;
    }

    [[nodiscard]] bool IsNumber() const noexcept {
        return kind_ == Kind::kNumber;
    }

    [[nodiscard]] const Object& AsObject() const {
        return object_value_;
    }

    [[nodiscard]] const Array& AsArray() const {
        return array_value_;
    }

    [[nodiscard]] const std::string& AsString() const {
        return string_value_;
    }

    [[nodiscard]] double AsNumber() const {
        return number_value_;
    }

private:
    Kind kind_{Kind::kNull};
    std::string string_value_;
    bool bool_value_{false};
    double number_value_{0.0};
    Object object_value_;
    Array array_value_;
};

class JsonParser final {
public:
    explicit JsonParser(std::string text)
        : text_(std::move(text)) {}

    JsonValue Parse() {
        SkipWhitespace();
        JsonValue value = ParseValue();
        SkipWhitespace();
        if (position_ != text_.size()) {
            throw std::runtime_error("Unexpected trailing JSON content");
        }
        return value;
    }

private:
    JsonValue ParseValue() {
        SkipWhitespace();
        if (position_ >= text_.size()) {
            throw std::runtime_error("Unexpected end of JSON input");
        }

        const char current = text_[position_];
        if (current == '{') {
            return ParseObject();
        }
        if (current == '[') {
            return ParseArray();
        }
        if (current == '"') {
            return JsonValue(ParseString());
        }
        if (text_.compare(position_, 4, "true") == 0) {
            position_ += 4;
            return JsonValue(true);
        }
        if (text_.compare(position_, 5, "false") == 0) {
            position_ += 5;
            return JsonValue(false);
        }
        if (text_.compare(position_, 4, "null") == 0) {
            position_ += 4;
            return JsonValue();
        }
        if (current == '-' || std::isdigit(static_cast<unsigned char>(current)) != 0) {
            return JsonValue(ParseNumber());
        }

        throw std::runtime_error("Unsupported JSON token");
    }

    JsonValue ParseObject() {
        Consume('{');
        JsonValue::Object object;
        SkipWhitespace();
        if (Peek('}')) {
            Consume('}');
            return JsonValue(std::move(object));
        }

        while (true) {
            SkipWhitespace();
            const std::string key = ParseString();
            SkipWhitespace();
            Consume(':');
            JsonValue value = ParseValue();
            object.emplace(key, std::move(value));
            SkipWhitespace();
            if (Peek('}')) {
                Consume('}');
                break;
            }
            Consume(',');
        }

        return JsonValue(std::move(object));
    }

    JsonValue ParseArray() {
        Consume('[');
        JsonValue::Array array;
        SkipWhitespace();
        if (Peek(']')) {
            Consume(']');
            return JsonValue(std::move(array));
        }

        while (true) {
            array.push_back(ParseValue());
            SkipWhitespace();
            if (Peek(']')) {
                Consume(']');
                break;
            }
            Consume(',');
        }

        return JsonValue(std::move(array));
    }

    std::string ParseString() {
        Consume('"');
        std::string value;
        while (position_ < text_.size()) {
            const char current = text_[position_++];
            if (current == '"') {
                return value;
            }
            if (current == '\\') {
                if (position_ >= text_.size()) {
                    throw std::runtime_error("Invalid JSON escape");
                }
                const char escaped = text_[position_++];
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
                        throw std::runtime_error("Unsupported JSON escape");
                }
                continue;
            }
            value.push_back(current);
        }

        throw std::runtime_error("Unterminated JSON string");
    }

    double ParseNumber() {
        const std::size_t start = position_;

        if (text_[position_] == '-') {
            ++position_;
        }
        while (position_ < text_.size() &&
               std::isdigit(static_cast<unsigned char>(text_[position_])) != 0) {
            ++position_;
        }
        if (position_ < text_.size() && text_[position_] == '.') {
            ++position_;
            while (position_ < text_.size() &&
                   std::isdigit(static_cast<unsigned char>(text_[position_])) != 0) {
                ++position_;
            }
        }
        if (position_ < text_.size() && (text_[position_] == 'e' || text_[position_] == 'E')) {
            ++position_;
            if (position_ < text_.size() && (text_[position_] == '+' || text_[position_] == '-')) {
                ++position_;
            }
            while (position_ < text_.size() &&
                   std::isdigit(static_cast<unsigned char>(text_[position_])) != 0) {
                ++position_;
            }
        }

        return std::stod(text_.substr(start, position_ - start));
    }

    void SkipWhitespace() {
        while (position_ < text_.size() &&
               std::isspace(static_cast<unsigned char>(text_[position_]))) {
            ++position_;
        }
    }

    void Consume(char expected) {
        SkipWhitespace();
        if (position_ >= text_.size() || text_[position_] != expected) {
            throw std::runtime_error("Unexpected JSON structure");
        }
        ++position_;
    }

    [[nodiscard]] bool Peek(char expected) const noexcept {
        return position_ < text_.size() && text_[position_] == expected;
    }

    std::string text_;
    std::size_t position_{0U};
};

const JsonValue& RequireObjectMember(const JsonValue::Object& object, const std::string& key) {
    const auto it = object.find(key);
    if (it == object.end()) {
        throw std::runtime_error("Missing JSON field: " + key);
    }
    return it->second;
}

std::string RequireStringMember(const JsonValue::Object& object, const std::string& key) {
    const JsonValue& value = RequireObjectMember(object, key);
    if (!value.IsString()) {
        throw std::runtime_error("JSON field is not a string: " + key);
    }
    return value.AsString();
}

const JsonValue::Array& RequireArrayMember(const JsonValue::Object& object,
                                           const std::string& key) {
    const JsonValue& value = RequireObjectMember(object, key);
    if (!value.IsArray()) {
        throw std::runtime_error("JSON field is not an array: " + key);
    }
    return value.AsArray();
}

const JsonValue::Object* FindObjectMember(const JsonValue::Object& object, const std::string& key) {
    const auto it = object.find(key);
    if (it == object.end() || !it->second.IsObject()) {
        return nullptr;
    }
    return &it->second.AsObject();
}

std::optional<int> FindIntegerMember(const JsonValue::Object& object, const std::string& key) {
    const auto it = object.find(key);
    if (it == object.end()) {
        return std::nullopt;
    }
    if (!it->second.IsNumber()) {
        throw std::runtime_error("JSON field is not a number: " + key);
    }
    return static_cast<int>(it->second.AsNumber());
}

std::string ReadTextFile(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input.is_open()) {
        throw std::runtime_error("Unable to open file: " + path.string());
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

struct FunctionGroupStateSelector {
    std::string function_group;
    std::string state;
};

struct StartupConfig {
    std::vector<std::string> process_arguments;
    std::vector<std::pair<std::string, std::string>> environment_variables;
    std::string termination_behavior;
};

struct StateDependentStartupConfig {
    std::vector<FunctionGroupStateSelector> function_group_states;
    StartupConfig startup_config;
};

struct ExecutionManifest {
    std::string short_name;
    std::string executable_name;
    std::filesystem::path executable_path;
    std::string reporting_behavior;
    std::vector<StateDependentStartupConfig> state_dependent_startup_configs;
};

struct MachineProcess {
    std::string name;
    ExecutionManifest execution_manifest;
};

struct MachineManifest {
    std::string initial_state;
    std::vector<MachineProcess> processes;
};

struct ExecutionManagerPlatformConfig {
    int reporting_timeout_ms{2000};
    int shutdown_grace_period_ms{1000};
};

std::vector<std::pair<std::string, std::string>>
ParseEnvironmentVariables(const JsonValue::Array& array) {
    std::vector<std::pair<std::string, std::string>> environment_variables;
    for (const JsonValue& value : array) {
        const auto& object = value.AsObject();
        environment_variables.emplace_back(RequireStringMember(object, "key"),
                                           RequireStringMember(object, "value"));
    }
    return environment_variables;
}

std::vector<std::string> ParseArguments(const JsonValue::Array& array) {
    std::vector<std::string> arguments;
    arguments.reserve(array.size());
    for (const JsonValue& value : array) {
        arguments.push_back(value.AsString());
    }
    return arguments;
}

std::vector<FunctionGroupStateSelector> ParseFunctionGroupStates(const JsonValue::Array& array) {
    std::vector<FunctionGroupStateSelector> selectors;
    for (const JsonValue& value : array) {
        const auto& object = value.AsObject();
        selectors.push_back(FunctionGroupStateSelector{
            .function_group = RequireStringMember(object, "functionGroup"),
            .state = RequireStringMember(object, "state"),
        });
    }
    return selectors;
}

std::optional<std::string> FindExecutableReportingBehavior(const JsonValue::Array& executables,
                                                           const std::string& executable_name) {
    for (const JsonValue& value : executables) {
        const auto& object = value.AsObject();
        if (RequireStringMember(object, "shortName") == executable_name) {
            return RequireStringMember(object, "reportingBehavior");
        }
    }

    return std::nullopt;
}

const JsonValue::Object& ResolveStartupConfig(const JsonValue::Array& startup_configs,
                                              const std::string& startup_config_ref) {
    const auto last_separator = startup_config_ref.find_last_of('/');
    const std::string startup_config_name = last_separator == std::string::npos
                                                ? startup_config_ref
                                                : startup_config_ref.substr(last_separator + 1U);

    for (const JsonValue& value : startup_configs) {
        const auto& object = value.AsObject();
        if (RequireStringMember(object, "shortName") == startup_config_name) {
            return object;
        }
    }

    throw std::runtime_error("StartupConfig not found: " + startup_config_ref);
}

ExecutionManifest ParseProcessExecutionManifest(const JsonValue::Object& process_object,
                                                const JsonValue::Array& executables,
                                                const JsonValue::Array& startup_configs,
                                                const std::filesystem::path& build_root) {
    ExecutionManifest execution_manifest;
    execution_manifest.short_name = RequireStringMember(process_object, "shortName");
    execution_manifest.executable_name = RequireStringMember(process_object, "executableName");
    execution_manifest.executable_path =
        build_root / RequireStringMember(process_object, "executablePath");
    execution_manifest.reporting_behavior =
        FindExecutableReportingBehavior(executables, execution_manifest.executable_name)
            .value_or("DOES-NOT-REPORT-EXECUTION-STATE");

    for (const JsonValue& value :
         RequireArrayMember(process_object, "stateDependentStartupConfigs")) {
        const auto& state_dependent_object = value.AsObject();
        const auto& startup_config_object = ResolveStartupConfig(
            startup_configs, RequireStringMember(state_dependent_object, "startupConfigRef"));

        execution_manifest.state_dependent_startup_configs.push_back(StateDependentStartupConfig{
            .function_group_states = ParseFunctionGroupStates(
                RequireArrayMember(state_dependent_object, "functionGroupStates")),
            .startup_config =
                StartupConfig{
                    .process_arguments = ParseArguments(
                        RequireArrayMember(startup_config_object, "processArguments")),
                    .environment_variables = ParseEnvironmentVariables(
                        RequireArrayMember(startup_config_object, "environmentVariables")),
                    .termination_behavior =
                        RequireStringMember(startup_config_object, "terminationBehavior"),
                },
        });
    }

    return execution_manifest;
}

std::vector<MachineProcess>
LoadProcessesFromExecutionManifest(const std::filesystem::path& manifest_path,
                                   const std::filesystem::path& build_root) {
    const JsonValue root = JsonParser(ReadTextFile(manifest_path)).Parse();
    const auto& root_object = root.AsObject();
    const auto& manifest_object = RequireObjectMember(root_object, "executionManifest").AsObject();
    const auto& processes = RequireArrayMember(manifest_object, "processes");
    const auto& executables = RequireArrayMember(manifest_object, "executables");
    const auto& startup_configs = RequireArrayMember(manifest_object, "startupConfigs");

    std::vector<MachineProcess> machine_processes;
    machine_processes.reserve(processes.size());
    for (const JsonValue& value : processes) {
        const auto& process_object = value.AsObject();
        ExecutionManifest execution_manifest =
            ParseProcessExecutionManifest(process_object, executables, startup_configs, build_root);
        machine_processes.push_back(MachineProcess{
            .name = execution_manifest.short_name,
            .execution_manifest = std::move(execution_manifest),
        });
    }

    return machine_processes;
}

ExecutionManagerPlatformConfig
LoadExecutionManagerPlatformConfig(const std::filesystem::path& manifest_path) {
    const JsonValue root = JsonParser(ReadTextFile(manifest_path)).Parse();
    const auto& root_object = root.AsObject();
    const auto& manifest_object = RequireObjectMember(root_object, "executionManifest").AsObject();

    ExecutionManagerPlatformConfig platform_config;
    const JsonValue::Object* platform_object = FindObjectMember(manifest_object, "platform");
    if (platform_object == nullptr) {
        return platform_config;
    }

    const JsonValue::Object* execution_manager_object =
        FindObjectMember(*platform_object, "executionManager");
    if (execution_manager_object == nullptr) {
        return platform_config;
    }

    platform_config.reporting_timeout_ms =
        FindIntegerMember(*execution_manager_object, "reportingTimeoutMs")
            .value_or(platform_config.reporting_timeout_ms);
    platform_config.shutdown_grace_period_ms =
        FindIntegerMember(*execution_manager_object, "shutdownGracePeriodMs")
            .value_or(platform_config.shutdown_grace_period_ms);
    return platform_config;
}

MachineManifest LoadMachineManifest(const std::filesystem::path& manifest_path,
                                    const std::filesystem::path&) {
    const JsonValue root = JsonParser(ReadTextFile(manifest_path)).Parse();
    const auto& root_object = root.AsObject();
    const auto& machine_object = RequireObjectMember(root_object, "machine").AsObject();

    MachineManifest machine_manifest;
    for (const JsonValue& function_group_value :
         RequireArrayMember(machine_object, "functionGroups")) {
        const auto& function_group_object = function_group_value.AsObject();
        if (RequireStringMember(function_group_object, "name") == kManagedFunctionGroup) {
            machine_manifest.initial_state =
                RequireStringMember(function_group_object, "initialState");
            break;
        }
    }
    if (machine_manifest.initial_state.empty()) {
        throw std::runtime_error("MachineFG initial state is not defined in machine manifest");
    }

    return machine_manifest;
}

bool HasMatchingStartupConfig(const MachineProcess& process, std::string_view machine_state) {
    for (const auto& config : process.execution_manifest.state_dependent_startup_configs) {
        for (const auto& selector : config.function_group_states) {
            if (selector.function_group == kManagedFunctionGroup &&
                selector.state == machine_state) {
                return true;
            }
        }
    }

    return false;
}

struct ManagedProcessRuntime {
    MachineProcess process;
    pid_t pid{-1};
    bool running_reported{false};
    bool reporting_timeout_logged{false};
    std::chrono::steady_clock::time_point started_at{};
};

class ExecutionManager final {
public:
    ExecutionManager(std::filesystem::path machine_manifest_path,
                     std::filesystem::path execution_manifest_path,
                     std::filesystem::path build_root)
        : machine_manifest_path_(std::move(machine_manifest_path)),
          execution_manifest_path_(std::move(execution_manifest_path)),
          build_root_(std::move(build_root)),
          logger_(std::cout) {}

    int Run() {
        logger_.Info("OPENAA",
                     std::string("Open Adaptive AUTOSAR version: ") +
                         OPEN_AA_VERSION); // OPEN_AA_VERSION is defined via compiler definition

        logger_.Info("OPENAA_EM", "Execution Manager starting up");

        try {
            machine_manifest_ = LoadMachineManifest(machine_manifest_path_, build_root_);
            machine_manifest_.processes =
                LoadProcessesFromExecutionManifest(execution_manifest_path_, build_root_);
            platform_config_ = LoadExecutionManagerPlatformConfig(execution_manifest_path_);
        } catch (const std::exception& exception) {
            logger_.Error("OPENAA_EM",
                          std::string("Failed to load manifests: ") + exception.what());
            return EXIT_FAILURE;
        }

        if (!SetupSignals() || !SetupExecutionReportSocket()) {
            return EXIT_FAILURE;
        }

        TransitionToMachineState(machine_manifest_.initial_state);
        StartProcessesForCurrentState();
        if (machine_manifest_.initial_state == "Startup") {
            TransitionToMachineState("Running");
            StartProcessesForCurrentState();
        }

        bool running = true;
        while (running) {
            std::array<pollfd, 2> descriptors{
                pollfd{.fd = signal_fd_, .events = POLLIN, .revents = 0},
                pollfd{.fd = report_socket_fd_, .events = POLLIN, .revents = 0},
            };

            const int result = poll(descriptors.data(), descriptors.size(), 100);
            if (result < 0) {
                if (errno == EINTR) {
                    continue;
                }
                logger_.Error("OPENAA_EM", "poll() failed");
                break;
            }

            if ((descriptors[0].revents & POLLIN) != 0) {
                running = HandleSignalEvent();
            }
            if ((descriptors[1].revents & POLLIN) != 0) {
                HandleExecutionReport();
            }
            CheckExecutionReportingTimeouts();
        }

        Shutdown();
        return EXIT_SUCCESS;
    }

private:
    bool SetupSignals() {
        sigset_t mask;
        sigemptyset(&mask);
        sigaddset(&mask, SIGINT);
        sigaddset(&mask, SIGTERM);
        sigaddset(&mask, SIGCHLD);

        if (sigprocmask(SIG_BLOCK, &mask, nullptr) == -1) {
            logger_.Error("OPENAA_EM", "Failed to block signals for signalfd");
            return false;
        }

        signal_fd_ = signalfd(-1, &mask, 0);
        if (signal_fd_ == -1) {
            logger_.Error("OPENAA_EM", "Failed to create signalfd");
            return false;
        }

        return true;
    }

    bool SetupExecutionReportSocket() {
        report_socket_path_ = (std::filesystem::temp_directory_path() /
                               ("openaa_em_" + std::to_string(getpid()) + ".sock"))
                                  .string();

        report_socket_fd_ = socket(AF_UNIX, SOCK_DGRAM, 0);
        if (report_socket_fd_ == -1) {
            logger_.Error("OPENAA_EM",
                          std::string("Failed to create execution report socket: ") +
                              std::strerror(errno));
            return false;
        }

        sockaddr_un address{};
        address.sun_family = AF_UNIX;
        std::snprintf(std::data(address.sun_path),
                      std::size(address.sun_path),
                      "%s",
                      report_socket_path_.c_str());

        unlink(report_socket_path_.c_str());
        if (bind(report_socket_fd_,
                 reinterpret_cast<const sockaddr*>(&address),
                 static_cast<socklen_t>(offsetof(sockaddr_un, sun_path) +
                                        report_socket_path_.size() + 1U)) == -1) {
            logger_.Error("OPENAA_EM",
                          std::string("Failed to bind execution report socket: ") +
                              std::strerror(errno));
            return false;
        }

        return true;
    }

    void TransitionToMachineState(const std::string& state) {
        machine_state_ = state;
        logger_.Info("OPENAA_EM", "Transitioning MachineFG to state '" + state + "'");
    }

    void StartProcessesForCurrentState() {
        for (const auto& process : machine_manifest_.processes) {
            if (!HasMatchingStartupConfig(process, machine_state_)) {
                continue;
            }

            if (!StartProcess(process)) {
                logger_.Error("OPENAA_EM",
                              std::string("Failed to start process '") + process.name + "'");
            }
        }
    }

    bool StartProcess(const MachineProcess& process) {
        std::vector<std::string> arguments;
        arguments.push_back(process.execution_manifest.executable_path.string());
        std::vector<std::pair<std::string, std::string>> environment_variables;
        environment_variables.emplace_back(kExecutionReportSocketEnv, report_socket_path_);
        environment_variables.emplace_back(kExecutionReportProcessEnv, process.name);

        for (const auto& config : process.execution_manifest.state_dependent_startup_configs) {
            for (const auto& selector : config.function_group_states) {
                if (selector.function_group == kManagedFunctionGroup &&
                    selector.state == machine_state_) {
                    arguments.insert(arguments.end(),
                                     config.startup_config.process_arguments.begin(),
                                     config.startup_config.process_arguments.end());
                    environment_variables.insert(
                        environment_variables.end(),
                        config.startup_config.environment_variables.begin(),
                        config.startup_config.environment_variables.end());
                }
            }
        }

        std::vector<char*> argv;
        argv.reserve(arguments.size() + 1U);
        for (std::string& argument : arguments) {
            argv.push_back(argument.data());
        }
        argv.push_back(nullptr);

        std::vector<std::string> env_storage;
        for (char** environment = environ; *environment != nullptr; ++environment) {
            env_storage.emplace_back(*environment);
        }
        for (const auto& [key, value] : environment_variables) {
            env_storage.push_back(key + "=" + value);
        }

        std::vector<char*> envp;
        envp.reserve(env_storage.size() + 1U);
        for (std::string& value : env_storage) {
            envp.push_back(value.data());
        }
        envp.push_back(nullptr);

        const pid_t child_pid = fork();
        if (child_pid == -1) {
            return false;
        }

        if (child_pid == 0) {
            // Ensure managed applications are terminated if Execution Manager exits unexpectedly.
            if (prctl(PR_SET_PDEATHSIG, SIGTERM) != 0) {
                _exit(127);
            }

            if (getppid() == 1) {
                _exit(127);
            }

            execve(process.execution_manifest.executable_path.c_str(), argv.data(), envp.data());
            _exit(127);
        }

        managed_processes_.emplace(child_pid,
                                   ManagedProcessRuntime{
                                       .process = process,
                                       .pid = child_pid,
                                       .running_reported = false,
                                       .reporting_timeout_logged = false,
                                       .started_at = std::chrono::steady_clock::now(),
                                   });
        logger_.Info("OPENAA_EM",
                     std::string("Started process '") + process.name + "' with pid " +
                         std::to_string(child_pid));
        return true;
    }

    bool HandleSignalEvent() {
        signalfd_siginfo info{};
        const ssize_t read_size = read(signal_fd_, &info, sizeof(info));
        if (read_size != sizeof(info)) {
            return true;
        }

        switch (info.ssi_signo) {
            case SIGINT:
            case SIGTERM:
                logger_.Info("OPENAA_EM", "Termination signal received");
                return false;
            case SIGCHLD:
                HandleChildExit();
                return true;
            default:
                logger_.Warn("OPENAA_EM", "Unexpected signal received");
                return true;
        }
    }

    void HandleExecutionReport() {
        std::array<char, 512> buffer{};
        const ssize_t bytes = recv(report_socket_fd_, buffer.data(), buffer.size() - 1, 0);
        if (bytes <= 0) {
            return;
        }

        buffer[static_cast<std::size_t>(bytes)] = '\0';
        const std::string payload(buffer.data());

        pid_t pid = -1;
        std::string process_name;
        std::string state;
        std::stringstream parser(payload);
        std::string token;
        while (std::getline(parser, token, ';')) {
            const auto separator = token.find('=');
            if (separator == std::string::npos) {
                continue;
            }
            const std::string key = token.substr(0, separator);
            const std::string value = token.substr(separator + 1);
            if (key == "pid") {
                pid = static_cast<pid_t>(std::stoi(value));
            } else if (key == "process") {
                process_name = value;
            } else if (key == "state") {
                state = value;
            }
        }

        const auto it = managed_processes_.find(pid);
        if (it == managed_processes_.end()) {
            logger_.Warn("OPENAA_EM", "Received execution report for unknown pid");
            return;
        }

        if (it->second.process.name != process_name) {
            logger_.Warn("OPENAA_EM", "Execution report process name mismatch");
            return;
        }

        if (state == "Running") {
            it->second.running_reported = true;
            logger_.Info("OPENAA_EM",
                         std::string("Process '") + process_name +
                             "' reported ExecutionState::kRunning");
        }
    }

    void HandleChildExit() {
        while (true) {
            int status = 0;
            const pid_t child_pid = waitpid(-1, &status, WNOHANG);
            if (child_pid <= 0) {
                break;
            }

            const auto it = managed_processes_.find(child_pid);
            if (it == managed_processes_.end()) {
                continue;
            }

            const bool exited_normally = WIFEXITED(status);
            const int exit_code = exited_normally ? WEXITSTATUS(status) : -1;
            logger_.Info("OPENAA_EM",
                         std::string("Managed process '") + it->second.process.name + "' exited" +
                             std::string(exited_normally ? " with code " + std::to_string(exit_code)
                                                         : " due to signal"));
            managed_processes_.erase(it);
        }
    }

    void CheckExecutionReportingTimeouts() {
        const auto now = std::chrono::steady_clock::now();
        for (auto& [pid, managed_process] : managed_processes_) {
            (void)pid;
            if (managed_process.running_reported || managed_process.reporting_timeout_logged ||
                managed_process.process.execution_manifest.reporting_behavior !=
                    "REPORTS-EXECUTION-STATE") {
                continue;
            }

            const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - managed_process.started_at);
            if (elapsed.count() < platform_config_.reporting_timeout_ms) {
                continue;
            }

            managed_process.reporting_timeout_logged = true;
            logger_.Error("OPENAA_EM",
                          std::string("Process '") + managed_process.process.name +
                              "' did not report ExecutionState::kRunning within " +
                              std::to_string(platform_config_.reporting_timeout_ms) + " ms");
        }
    }

    void Shutdown() {
        TransitionToMachineState("Shutdown");

        for (const auto& [pid, managed_process] : managed_processes_) {
            logger_.Info("OPENAA_EM",
                         std::string("Sending SIGTERM to '") + managed_process.process.name + "'");
            kill(pid, SIGTERM);
        }

        const int sleep_step_ms = 100;
        const int max_attempts =
            std::max(1, platform_config_.shutdown_grace_period_ms / sleep_step_ms);
        for (int attempt = 0; attempt < max_attempts && !managed_processes_.empty(); ++attempt) {
            HandleChildExit();
            if (!managed_processes_.empty()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(sleep_step_ms));
            }
        }

        if (!managed_processes_.empty()) {
            for (const auto& [pid, managed_process] : managed_processes_) {
                logger_.Warn("OPENAA_EM",
                             std::string("Process '") + managed_process.process.name +
                                 "' did not terminate in time, sending SIGKILL");
                kill(pid, SIGKILL);
            }

            for (int attempt = 0; attempt < max_attempts && !managed_processes_.empty();
                 ++attempt) {
                HandleChildExit();
                if (!managed_processes_.empty()) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(sleep_step_ms));
                }
            }
        }

        if (signal_fd_ != -1) {
            close(signal_fd_);
        }
        if (report_socket_fd_ != -1) {
            close(report_socket_fd_);
        }
        if (!report_socket_path_.empty()) {
            unlink(report_socket_path_.c_str());
        }

        logger_.Info("OPENAA_EM", "Execution Manager shutdown complete");
    }

    std::filesystem::path machine_manifest_path_;
    std::filesystem::path execution_manifest_path_;
    std::filesystem::path build_root_;
    ara::log::Logger logger_;
    MachineManifest machine_manifest_{};
    ExecutionManagerPlatformConfig platform_config_{};
    std::map<pid_t, ManagedProcessRuntime> managed_processes_;
    std::string machine_state_;
    int signal_fd_{-1};
    int report_socket_fd_{-1};
    std::string report_socket_path_;
};

std::filesystem::path ResolveBuildRoot(const std::filesystem::path& executable_path) {
    return executable_path.parent_path().parent_path();
}

} // namespace

int main(int argc, char* argv[]) {
    auto init_result = ara::core::Initialize(argc, argv);
    if (!init_result.HasValue()) {
        std::cerr << "Failed to initialize ara::core: " << init_result.Error().Message()
                  << std::endl;
        return EXIT_FAILURE;
    }

    const std::filesystem::path executable_path =
        std::filesystem::weakly_canonical(std::filesystem::path(argv[0]));
    const std::filesystem::path build_root = ResolveBuildRoot(executable_path);
    const std::filesystem::path machine_manifest_path =
        argc > 1 ? std::filesystem::path(argv[1])
                 : build_root / "manifests" / "machine_manifest.json";
    const std::filesystem::path execution_manifest_path =
        argc > 2 ? std::filesystem::path(argv[2])
                 : build_root / "manifests" / "execution_manifest.json";

    ExecutionManager execution_manager(machine_manifest_path, execution_manifest_path, build_root);
    const int exit_code = execution_manager.Run();

    (void)ara::core::Deinitialize();
    return exit_code;
}
