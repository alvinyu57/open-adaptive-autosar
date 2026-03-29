#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>

#include "ara/core/application.hpp"
#include "ara/exec/execution_manager.hpp"

namespace {

class TestApp final : public ara::core::Application {
public:
    TestApp()
        : ara::core::Application("tests.smoke") {}

private:
    void OnInitialize(ara::core::RuntimeContext& context) override {
        const bool inserted = context.Services().Register({
            .service_id = "tests.smoke.service",
            .endpoint = "local://tests-smoke",
            .owner = Name(),
        });

        if (!inserted) {
            throw std::runtime_error("service registration failed");
        }
    }

    void OnStart(ara::core::RuntimeContext&) override {}

    void OnStop(ara::core::RuntimeContext&) override {}
};

} // namespace

int main() {
    const auto manifest_path =
        std::filesystem::temp_directory_path() / "openaa_core_smoke_manifest.json";
    {
        std::ofstream output(manifest_path);
        output << R"({
            "applicationId": "tests.smoke",
            "shortName": "tests.smoke",
            "executable": "openaa_core_smoke_test",
            "autoStart": true
        })";
    }

    ara::core::Logger logger(std::cout);
    ara::exec::ExecutionManager manager(logger);
    manager.RegisterApplicationFactory("tests.smoke", []() {
        return std::make_unique<TestApp>();
    });

    manager.LoadApplicationManifest(manifest_path.string());
    manager.Start();

    const auto services = manager.Services().List();
    if (services.size() != 1 || services.front().service_id != "tests.smoke.service") {
        std::cerr << "unexpected service registry state\n";
        return 1;
    }

    manager.Stop();
    std::filesystem::remove(manifest_path);
    return 0;
}
