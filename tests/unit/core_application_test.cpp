#include <sstream>
#include <stdexcept>
#include <string>

#include <gtest/gtest.h>

#include "openaa/core/application.hpp"

namespace {

class RecordingApplication final : public openaa::core::Application {
public:
    RecordingApplication() : openaa::core::Application("tests.unit.recording") {}

    bool initialized{false};
    bool started{false};
    bool stopped{false};
    std::size_t registered_services{0U};

private:
    void OnInitialize(openaa::core::RuntimeContext& context) override {
        initialized = true;
        const bool inserted = context.Services().Register({
            .service_id = "tests.unit.recording.service",
            .endpoint = "local://tests-unit",
            .owner = Name(),
        });

        if (!inserted) {
            throw std::runtime_error("service registration failed");
        }

        registered_services = context.Services().List().size();
    }

    void OnStart(openaa::core::RuntimeContext&) override {
        started = true;
    }

    void OnStop(openaa::core::RuntimeContext&) override {
        stopped = true;
    }
};

TEST(ServiceRegistryTest, RejectsDuplicateServiceId) {
    openaa::core::ServiceRegistry registry;

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
    openaa::core::Logger logger(logs);
    openaa::core::ServiceRegistry registry;
    openaa::core::RuntimeContext context(registry, logger);
    RecordingApplication application;

    EXPECT_EQ(application.State(), openaa::core::ApplicationState::kCreated);

    application.Initialize(context);
    EXPECT_TRUE(application.initialized);
    EXPECT_EQ(application.registered_services, 1U);
    EXPECT_EQ(application.State(), openaa::core::ApplicationState::kInitialized);

    application.Run(context);
    EXPECT_TRUE(application.started);
    EXPECT_EQ(application.State(), openaa::core::ApplicationState::kRunning);

    application.Stop(context);
    EXPECT_TRUE(application.stopped);
    EXPECT_EQ(application.State(), openaa::core::ApplicationState::kStopped);
}

TEST(ApplicationTest, RejectsRunningBeforeInitialization) {
    std::ostringstream logs;
    openaa::core::Logger logger(logs);
    openaa::core::ServiceRegistry registry;
    openaa::core::RuntimeContext context(registry, logger);
    RecordingApplication application;

    EXPECT_THROW(application.Run(context), std::logic_error);
}

TEST(ApplicationTest, StopBeforeInitializationIsANoop) {
    std::ostringstream logs;
    openaa::core::Logger logger(logs);
    openaa::core::ServiceRegistry registry;
    openaa::core::RuntimeContext context(registry, logger);
    RecordingApplication application;

    application.Stop(context);

    EXPECT_FALSE(application.stopped);
    EXPECT_EQ(application.State(), openaa::core::ApplicationState::kCreated);
}

}  // namespace
