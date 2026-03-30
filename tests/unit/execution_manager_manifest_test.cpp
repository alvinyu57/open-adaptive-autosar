#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include <gtest/gtest.h>

#include "ara/core/application.hpp"
#include "ara/exec/execution_manager.hpp"
#include "ara/log/logger.hpp"

namespace {

class ManifestApplication final : public ara::core::Application {
public:
    ManifestApplication()
        : ara::core::Application("tests.unit.manifested") {}

private:
    void OnInitialize(ara::core::RuntimeContext& context) override {
        const bool inserted = context.Services().Register({
            .service_id = "tests.unit.manifested.service",
            .endpoint = "local://manifested",
            .owner = Name(),
        });

        if (!inserted) {
            throw std::runtime_error("service registration failed");
        }
    }

    void OnStart(ara::core::RuntimeContext&) override {}

    void OnStop(ara::core::RuntimeContext&) override {}
};

std::filesystem::path WriteManifestFile(std::string_view contents, std::string_view name) {
    const auto path = std::filesystem::temp_directory_path() / std::string(name);
    std::ofstream output(path);
    output << contents;
    output.close();
    return path;
}

TEST(ExecutionManagerManifestTest, LoadsManifestAndStartsRegisteredApplication) {
    const auto manifest_path = WriteManifestFile(
        R"({
            "applicationId": "tests.unit.manifested",
            "shortName": "tests.unit.manifested",
            "executable": "openaa_core_unit_test",
            "arguments": ["--demo"],
            "environmentVariables": {
                "OPENAA_PROFILE": "unit"
            },
            "providedServices": [
                {
                    "serviceId": "tests.unit.manifested.service",
                    "endpoint": "local://manifested"
                }
            ],
            "autoStart": true
        })",
        "openaa_manifest_success.json");

    std::ostringstream logs;
    ara::log::Logger logger(logs);
    ara::exec::ExecutionManager manager(logger);
    manager.RegisterApplicationFactory("tests.unit.manifested", []() {
        return std::make_unique<ManifestApplication>();
    });

    manager.LoadApplicationManifest(manifest_path.string());
    const auto manifests = manager.LoadedManifests();
    ASSERT_EQ(manifests.size(), 1U);
    EXPECT_EQ(manifests.front().arguments.size(), 1U);
    EXPECT_EQ(manifests.front().environment_variables.at("OPENAA_PROFILE"), "unit");
    EXPECT_EQ(manifests.front().provided_services.front().service_id,
              "tests.unit.manifested.service");

    manager.Start();

    const auto services = manager.Services().List();
    ASSERT_EQ(services.size(), 1U);
    EXPECT_EQ(services.front().owner, "tests.unit.manifested");

    manager.Stop();

    std::filesystem::remove(manifest_path);
}

TEST(ExecutionManagerManifestTest, RejectsManifestWithoutRegisteredFactory) {
    const auto manifest_path = WriteManifestFile(
        R"({
            "applicationId": "tests.unit.unknown",
            "shortName": "tests.unit.unknown",
            "executable": "openaa_core_unit_test",
            "autoStart": true
        })",
        "openaa_manifest_missing_factory.json");

    std::ostringstream logs;
    ara::log::Logger logger(logs);
    ara::exec::ExecutionManager manager(logger);

    EXPECT_THROW(manager.LoadApplicationManifest(manifest_path.string()), std::runtime_error);

    std::filesystem::remove(manifest_path);
}

TEST(ExecutionManagerManifestTest, DoesNotAutoStartWhenDisabledInManifest) {
    const auto manifest_path = WriteManifestFile(
        R"({
            "applicationId": "tests.unit.manifested",
            "shortName": "tests.unit.manifested",
            "executable": "openaa_core_unit_test",
            "autoStart": false
        })",
        "openaa_manifest_autostart_false.json");

    std::ostringstream logs;
    ara::log::Logger logger(logs);
    ara::exec::ExecutionManager manager(logger);
    manager.RegisterApplicationFactory("tests.unit.manifested", []() {
        return std::make_unique<ManifestApplication>();
    });

    manager.LoadApplicationManifest(manifest_path.string());
    manager.Start();

    EXPECT_TRUE(manager.Services().List().empty());
    EXPECT_NE(logs.str().find("Skip autostart"), std::string::npos);

    std::filesystem::remove(manifest_path);
}

} // namespace
