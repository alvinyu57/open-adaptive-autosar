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
#include <unistd.h>
#include <utility>
#include <vector>

#include "ara/core/initialization.h"
#include "ara/log/logger.hpp"

extern char** environ;

namespace {

constexpr char kExecutionReportSocketEnv[] = "OPENAA_EXEC_REPORT_SOCKET";
constexpr char kExecutionReportProcessEnv[] = "OPENAA_EXEC_PROCESS_NAME";
constexpr std::string_view kManagedFunctionGroup = "MachineFG";

class JsonValue final {
public:
    using Object = std::map<std::string, JsonValue>;
    using Array = std::vector<JsonValue>;

    enum class Kind { kNull, kString, kBool, kObject, kArray };

    JsonValue() = default;
    explicit JsonValue(std::string value)
        : kind_(Kind::kString),
          string_value_(std::move(value)) {}
    explicit JsonValue(bool value)
        : kind_(Kind::kBool),
          bool_value_(value) {}
    explicit JsonValue(Object value)
        : kind_(Kind::kObject),
          object_value_(std::move(value)) {}
    explicit JsonValue(Array value)
        : kind_(Kind::kArray),
          array_value_(std::move(value)) {}

    [[nodiscard]] bool IsObject() const noexcept { return kind_ == Kind::kObject; }
    [[nodiscard]] bool IsArray() const noexcept { return kind_ == Kind::kArray; }
    [[nodiscard]] bool IsString() const noexcept { return kind_ == Kind::kString; }
    [[nodiscard]] const Object& AsObject() const { return object_value_; }
    [[nodiscard]] const Array& AsArray() const { return array_value_; }
    [[nodiscard]] const std::string& AsString() const { return string_value_; }

private:
    Kind kind_{Kind::kNull};
    std::string string_value_;
    bool bool_value_{false};
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

    void SkipWhitespace() {
        while (position_ < text_.size() && std::isspace(static_cast<unsigned char>(text_[position_]))) {
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

const JsonValue::Array& RequireArrayMember(const JsonValue::Object& object, const std::string& key) {
    const JsonValue& value = RequireObjectMember(object, key);
    if (!value.IsArray()) {
        throw std::runtime_error("JSON field is not an array: " + key);
    }
    return value.AsArray();
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
    std::vector<std::string> arguments;
    std::vector<std::pair<std::string, std::string>> environment_variables;
    std::string termination_behavior;
};

struct StateDependentStartupConfig {
    std::vector<FunctionGroupStateSelector> function_group_states;
    StartupConfig startup_config;
};

struct ExecutionManifest {
    std::string short_name;
    std::string executable;
    std::string reporting_behavior;
    std::vector<std::string> arguments;
    std::vector<std::pair<std::string, std::string>> environment_variables;
    std::vector<StateDependentStartupConfig> state_dependent_startup_configs;
};

struct MachineProcess {
    std::string name;
    std::filesystem::path executable_path;
    std::filesystem::path execution_manifest_path;
    std::vector<FunctionGroupStateSelector> startup_configs;
    ExecutionManifest execution_manifest;
};

struct MachineManifest {
    std::string initial_state;
    std::vector<MachineProcess> processes;
};

std::vector<std::pair<std::string, std::string>> ParseEnvironmentVariables(const JsonValue::Array& array) {
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

ExecutionManifest LoadExecutionManifest(const std::filesystem::path& path) {
    const JsonValue root = JsonParser(ReadTextFile(path)).Parse();
    const auto& root_object = root.AsObject();
    const auto& manifest_object = RequireObjectMember(root_object, "executionManifest").AsObject();
    const auto& process_object = RequireObjectMember(manifest_object, "process").AsObject();

    ExecutionManifest execution_manifest;
    execution_manifest.short_name = RequireStringMember(process_object, "shortName");
    execution_manifest.executable = RequireStringMember(process_object, "executable");
    execution_manifest.reporting_behavior = RequireStringMember(process_object, "reportingBehavior");
    execution_manifest.arguments =
        ParseArguments(RequireArrayMember(process_object, "arguments"));
    execution_manifest.environment_variables =
        ParseEnvironmentVariables(RequireArrayMember(process_object, "environmentVariables"));

    for (const JsonValue& value :
         RequireArrayMember(process_object, "stateDependentStartupConfigs")) {
        const auto& state_dependent_object = value.AsObject();
        const auto& startup_config_object =
            RequireObjectMember(state_dependent_object, "startupConfig").AsObject();

        execution_manifest.state_dependent_startup_configs.push_back(StateDependentStartupConfig{
            .function_group_states =
                ParseFunctionGroupStates(
                    RequireArrayMember(state_dependent_object, "functionGroupStates")),
            .startup_config =
                StartupConfig{
                    .arguments =
                        ParseArguments(RequireArrayMember(startup_config_object, "arguments")),
                    .environment_variables = ParseEnvironmentVariables(
                        RequireArrayMember(startup_config_object, "environmentVariables")),
                    .termination_behavior =
                        RequireStringMember(startup_config_object, "terminationBehavior"),
                },
        });
    }

    return execution_manifest;
}

MachineManifest LoadMachineManifest(const std::filesystem::path& manifest_path,
                                    const std::filesystem::path& build_root) {
    const JsonValue root = JsonParser(ReadTextFile(manifest_path)).Parse();
    const auto& root_object = root.AsObject();
    const auto& machine_object = RequireObjectMember(root_object, "machine").AsObject();

    MachineManifest manifest;
    for (const JsonValue& function_group_value :
         RequireArrayMember(machine_object, "functionGroups")) {
        const auto& function_group_object = function_group_value.AsObject();
        if (RequireStringMember(function_group_object, "name") == kManagedFunctionGroup) {
            manifest.initial_state = RequireStringMember(function_group_object, "initialState");
            break;
        }
    }
    if (manifest.initial_state.empty()) {
        throw std::runtime_error("MachineFG initial state is not defined in machine manifest");
    }

    for (const JsonValue& process_value : RequireArrayMember(machine_object, "processes")) {
        const auto& process_object = process_value.AsObject();

        MachineProcess process;
        process.name = RequireStringMember(process_object, "name");
        process.executable_path = build_root / RequireStringMember(process_object, "executablePath");
        process.execution_manifest_path =
            build_root / RequireStringMember(process_object, "executionManifest");
        process.startup_configs =
            ParseFunctionGroupStates(RequireArrayMember(process_object, "startupConfigs"));
        process.execution_manifest = LoadExecutionManifest(process.execution_manifest_path);

        manifest.processes.push_back(std::move(process));
    }

    return manifest;
}

bool HasMatchingStartupConfig(const MachineProcess& process, std::string_view machine_state) {
    for (const auto& selector : process.startup_configs) {
        if (selector.function_group == kManagedFunctionGroup && selector.state == machine_state) {
            return true;
        }
    }

    for (const auto& config : process.execution_manifest.state_dependent_startup_configs) {
        for (const auto& selector : config.function_group_states) {
            if (selector.function_group == kManagedFunctionGroup && selector.state == machine_state) {
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
};

class ExecutionManager final {
public:
    ExecutionManager(std::filesystem::path manifest_path, std::filesystem::path build_root)
        : manifest_path_(std::move(manifest_path)),
          build_root_(std::move(build_root)),
          logger_(std::cout) {}

    int Run() {
        logger_.Info("EM", "Execution Manager starting up");

        try {
            machine_manifest_ = LoadMachineManifest(manifest_path_, build_root_);
        } catch (const std::exception& exception) {
            logger_.Error("EM", std::string("Failed to load manifests: ") + exception.what());
            return EXIT_FAILURE;
        }

        if (!SetupSignals() || !SetupExecutionReportSocket()) {
            return EXIT_FAILURE;
        }

        TransitionToMachineState(machine_manifest_.initial_state);
        StartProcessesForCurrentState();

        bool running = true;
        while (running) {
            std::array<pollfd, 2> descriptors{
                pollfd{.fd = signal_fd_, .events = POLLIN, .revents = 0},
                pollfd{.fd = report_socket_fd_, .events = POLLIN, .revents = 0},
            };

            const int result = poll(descriptors.data(), descriptors.size(), -1);
            if (result < 0) {
                if (errno == EINTR) {
                    continue;
                }
                logger_.Error("EM", "poll() failed");
                break;
            }

            if ((descriptors[0].revents & POLLIN) != 0) {
                running = HandleSignalEvent();
            }
            if ((descriptors[1].revents & POLLIN) != 0) {
                HandleExecutionReport();
            }
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
            logger_.Error("EM", "Failed to block signals for signalfd");
            return false;
        }

        signal_fd_ = signalfd(-1, &mask, 0);
        if (signal_fd_ == -1) {
            logger_.Error("EM", "Failed to create signalfd");
            return false;
        }

        return true;
    }

    bool SetupExecutionReportSocket() {
        report_socket_path_ =
            (std::filesystem::temp_directory_path() /
             ("openaa_em_" + std::to_string(getpid()) + ".sock"))
                .string();

        report_socket_fd_ = socket(AF_UNIX, SOCK_DGRAM, 0);
        if (report_socket_fd_ == -1) {
            logger_.Error("EM",
                          std::string("Failed to create execution report socket: ") +
                              std::strerror(errno));
            return false;
        }

        sockaddr_un address{};
        address.sun_family = AF_UNIX;
        std::snprintf(address.sun_path, sizeof(address.sun_path), "%s", report_socket_path_.c_str());

        unlink(report_socket_path_.c_str());
        if (bind(report_socket_fd_,
                 reinterpret_cast<const sockaddr*>(&address),
                 static_cast<socklen_t>(offsetof(sockaddr_un, sun_path) +
                                        report_socket_path_.size() + 1U)) == -1) {
            logger_.Error("EM",
                          std::string("Failed to bind execution report socket: ") +
                              std::strerror(errno));
            return false;
        }

        return true;
    }

    void TransitionToMachineState(const std::string& state) {
        machine_state_ = state;
        logger_.Info("EM", "Transitioning MachineFG to state '" + state + "'");
    }

    void StartProcessesForCurrentState() {
        for (const auto& process : machine_manifest_.processes) {
            if (!HasMatchingStartupConfig(process, machine_state_)) {
                continue;
            }

            if (!StartProcess(process)) {
                logger_.Error("EM", std::string("Failed to start process '") + process.name + "'");
            }
        }
    }

    bool StartProcess(const MachineProcess& process) {
        std::vector<std::string> arguments;
        arguments.push_back(process.executable_path.string());
        arguments.insert(arguments.end(),
                         process.execution_manifest.arguments.begin(),
                         process.execution_manifest.arguments.end());

        std::vector<std::pair<std::string, std::string>> environment_variables =
            process.execution_manifest.environment_variables;
        environment_variables.emplace_back(kExecutionReportSocketEnv, report_socket_path_);
        environment_variables.emplace_back(kExecutionReportProcessEnv, process.name);

        for (const auto& config : process.execution_manifest.state_dependent_startup_configs) {
            for (const auto& selector : config.function_group_states) {
                if (selector.function_group == kManagedFunctionGroup &&
                    selector.state == machine_state_) {
                    arguments.insert(arguments.end(),
                                     config.startup_config.arguments.begin(),
                                     config.startup_config.arguments.end());
                    environment_variables.insert(environment_variables.end(),
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
            execve(process.executable_path.c_str(), argv.data(), envp.data());
            _exit(127);
        }

        managed_processes_.emplace(child_pid,
                                   ManagedProcessRuntime{
                                       .process = process,
                                       .pid = child_pid,
                                       .running_reported = false,
                                   });
        logger_.Info("EM",
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
                logger_.Info("EM", "Termination signal received");
                return false;
            case SIGCHLD:
                HandleChildExit();
                return true;
            default:
                logger_.Warn("EM", "Unexpected signal received");
                return true;
        }
    }

    void HandleExecutionReport() {
        std::array<char, 512> buffer{};
        const ssize_t bytes =
            recv(report_socket_fd_, buffer.data(), buffer.size() - 1, 0);
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
            logger_.Warn("EM", "Received execution report for unknown pid");
            return;
        }

        if (it->second.process.name != process_name) {
            logger_.Warn("EM", "Execution report process name mismatch");
            return;
        }

        if (state == "Running") {
            it->second.running_reported = true;
            logger_.Info("EM",
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
            logger_.Info("EM",
                         std::string("Managed process '") + it->second.process.name + "' exited" +
                             std::string(exited_normally
                                             ? " with code " + std::to_string(exit_code)
                                             : " due to signal"));
            managed_processes_.erase(it);
        }
    }

    void Shutdown() {
        TransitionToMachineState("Shutdown");

        for (const auto& [pid, managed_process] : managed_processes_) {
            logger_.Info("EM",
                         std::string("Sending SIGTERM to '") + managed_process.process.name +
                             "'");
            kill(pid, SIGTERM);
        }

        for (int attempt = 0; attempt < 10 && !managed_processes_.empty(); ++attempt) {
            HandleChildExit();
            if (!managed_processes_.empty()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
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

        logger_.Info("EM", "Execution Manager shutdown complete");
    }

    std::filesystem::path manifest_path_;
    std::filesystem::path build_root_;
    ara::log::Logger logger_;
    MachineManifest machine_manifest_{};
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
    const std::filesystem::path manifest_path =
        argc > 1 ? std::filesystem::path(argv[1]) : build_root / "manifests" / "machine_manifest.json";

    ExecutionManager execution_manager(manifest_path, build_root);
    const int exit_code = execution_manager.Run();

    (void)ara::core::Deinitialize();
    return exit_code;
}
