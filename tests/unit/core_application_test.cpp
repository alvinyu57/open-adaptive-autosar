#include <sstream>
#include <stdexcept>
#include <string>

#include <gtest/gtest.h>

#include "ara/exec/application.hpp"
#include "ara/log/logger.hpp"

namespace {

class RecordingApplication final : public ara::exec::Application {
public:
    RecordingApplication()
        : ara::exec::Application("tests.unit.recording") {}

    bool initialized{false};
    bool started{false};
    bool stopped{false};
    std::size_t registered_services{0U};

private:
    void OnInitialize(ara::exec::RuntimeContext& context) override {
        initialized = true;
        bool inserted{false};
        inserted = context.Services().Register({
            .service_id = "tests.unit.recording.service",
            .endpoint = "local://tests-unit",
            .owner = Name(),
        });

        if (!inserted) {
            throw std::runtime_error("service registration failed");
        }

        registered_services = context.Services().List().size();
    }

    void OnStart(ara::exec::RuntimeContext&) override {
        started = true;
    }

    void OnStop(ara::exec::RuntimeContext&) override {
        stopped = true;
    }
};

TEST(ServiceRegistryTest, RejectsDuplicateServiceId) {
    ara::exec::ServiceRegistry registry;

    EXPECT_TRUE(registry.Register({
        .service_id = "svc.alpha",
        .endpoint = "local://alpha",
        .owner = "app.alpha",
    }));

    EXPECT_FALSE(registry.Register({
        .service_id = "svc.alpha",
        .endpoint = "local://alpha-2",
        .owner = "app.beta",
    }));

    const auto services = registry.List();
    ASSERT_EQ(services.size(), 1U);
    EXPECT_EQ(services.front().owner, "app.alpha");
}

TEST(ApplicationTest, TransitionsThroughLifecycleAndInvokesHooks) {
    std::ostringstream logs;
    ara::log::Logger logger(logs);
    ara::exec::ServiceRegistry registry;
    ara::exec::RuntimeContext context(registry, logger);
    RecordingApplication application;

    EXPECT_EQ(application.State(), ara::exec::ApplicationState::kCreated);

    application.Initialize(context);
    EXPECT_TRUE(application.initialized);
    EXPECT_EQ(application.registered_services, 1U);
    EXPECT_EQ(application.State(), ara::exec::ApplicationState::kInitialized);

    application.Run(context);
    EXPECT_TRUE(application.started);
    EXPECT_EQ(application.State(), ara::exec::ApplicationState::kRunning);

    application.Stop(context);
    EXPECT_TRUE(application.stopped);
    EXPECT_EQ(application.State(), ara::exec::ApplicationState::kStopped);
}

TEST(ApplicationTest, RejectsRunningBeforeInitialization) {
    std::ostringstream logs;
    ara::log::Logger logger(logs);
    ara::exec::ServiceRegistry registry;
    ara::exec::RuntimeContext context(registry, logger);
    RecordingApplication application;

    EXPECT_THROW(application.Run(context), std::logic_error);
}

TEST(ApplicationTest, StopBeforeInitializationIsANoop) {
    std::ostringstream logs;
    ara::log::Logger logger(logs);
    ara::exec::ServiceRegistry registry;
    ara::exec::RuntimeContext context(registry, logger);
    RecordingApplication application;

    application.Stop(context);

    EXPECT_FALSE(application.stopped);
    EXPECT_EQ(application.State(), ara::exec::ApplicationState::kCreated);
}

} // namespace
