#include <array>
#include <csignal>
#include <iostream>
#include <string_view>
#include <sys/signalfd.h>
#include <unistd.h>
#include <vector>

#include "ara/core/initialization.h"
#include "ara/core/result.h"
#include "ara/exec/execution_client.h"
#include "ara/log/logger.hpp"
#include "em_runtime_state.hpp"

namespace {

/**
 * @brief Simple internal structure to represent a process entry in the Machine Manifest.
 */
struct ProcessManifestEntry {
    std::string_view name;
    std::string_view executable_path;
    // In a real implementation, this would include arguments, environmental variables, resource limits, etc.
};

/**
 * @brief The Execution Management Engine.
 * 
 * Responsible for overseeing the system lifecycle, managing Machine State transitions,
 * and monitoring the status of all managed process instances.
 */
class ExecutionManager final {
public:
    ExecutionManager()
        : logger_(std::cout),
          machine_state_("Startup") {}

    /**
     * @brief Run the Execution Manager.
     */
    void Run() {
        logger_.Info("EM", "Execution Manager starting up...");

        if (SetupSignals() != 0) {
            logger_.Error("EM", "Failed to setup signal handling.");
            return;
        }

        // 1. Initial State: Startup
        // According to SWS_EM_01001, EM shall transition the Machine to the Startup state.
        TransitionToMachineState("Startup");

        // 2. Start Initial processes
        // In a real AP system, these are defined in the Machine Manifest.
        StartInitialProcesses();

        // 3. Main Event Loop
        bool running = true;
        while (running) {
            signalfd_siginfo fdsi;
            ssize_t s = read(signal_fd_, &fdsi, sizeof(fdsi));
            if (s != sizeof(fdsi)) {
                continue;
            }

            switch (fdsi.ssi_signo) {
                case SIGINT:
                case SIGTERM:
                    logger_.Info("EM", "Termination signal received. Initiating shutdown...");
                    running = false;
                    break;

                case SIGCHLD:
                    HandleChildExit();
                    break;

                default:
                    logger_.Warn("EM", "Received unexpected signal.");
                    break;
            }
        }

        Shutdown();
    }

private:
    int SetupSignals() {
        sigset_t mask;
        sigemptyset(&mask);
        sigaddset(&mask, SIGINT);
        sigaddset(&mask, SIGTERM);
        sigaddset(&mask, SIGCHLD);

        // Block signals so they can be handled by signalfd
        if (sigprocmask(SIG_BLOCK, &mask, nullptr) == -1) {
            return -1;
        }

        signal_fd_ = signalfd(-1, &mask, 0);
        return (signal_fd_ == -1) ? -1 : 0;
    }

    void TransitionToMachineState(std::string_view state) {
        logger_.Info("EM", std::string("Transitioning Machine State to: ") + std::string(state));
        machine_state_ = state;
        
        // In a real system, this triggers starting/stopping apps based on the manifest.
        // Also notifies StateClient consumers via the internal/RuntimeState.
    }

    void StartInitialProcesses() {
        // Placeholder for process spawning logic as per SWS_EM_01021.
        // "Execution Management shall start Processes which have at least one Startup Configuration
        // with the Machine State to which the transition is being performed."
        logger_.Info("EM", "Spawning initial processes defined in Machine Manifest...");
    }

    void HandleChildExit() {
        // Reaper for child processes (waitpid loop)
        logger_.Info("EM", "A managed process has exited.");
    }

    void Shutdown() {
        TransitionToMachineState("Shutdown");
        logger_.Info("EM", "Execution Manager shutdown complete.");
        if (signal_fd_ != -1) {
            close(signal_fd_);
        }
    }

    ara::log::Logger logger_;
    std::string_view machine_state_;
    int signal_fd_{-1};
};

} // namespace

int main(int argc, char* argv[]) {
    // SWS_CORE_00001: ara::core::Initialize() shall be called before any other AP API.
    auto init_result = ara::core::Initialize(argc, argv);
    if (!init_result) {
        std::cerr << "Failed to initialize ara::core: " << init_result.Error().Message() << std::endl;
        return 1;
    }

    {
        ExecutionManager em;
        em.Run();
    }

    // SWS_CORE_00002: ara::core::Deinitialize() shall be called before process termination.
    (void)ara::core::Deinitialize();

    return 0;
}
