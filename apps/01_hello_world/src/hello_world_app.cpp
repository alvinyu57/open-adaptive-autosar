#include "openaa/examples/hello_world/hello_world_app.hpp"

#include <sstream>

namespace openaa::examples::hello_world {

namespace {

class HelloWorldApp final : public ara::exec::Application {
public:
    HelloWorldApp()
        : ara::exec::Application("examples.hello_world") {}

private:
    void OnInitialize(ara::exec::RuntimeContext& context) override {
        const bool inserted = context.Services().Register({
            .service_id = "examples.hello.greeter",
            .endpoint = "local://hello-greeter",
            .owner = Name(),
        });

        if (!inserted) {
            context.Log().Warn(Name(), "Greeter service already registered");
            return;
        }

        context.Log().Info(Name(), "Registered greeter service");
    }

    void OnStart(ara::exec::RuntimeContext& context) override {
        context.Log().Info(Name(), "Offer greeting: Hello from Adaptive AUTOSAR MVP");
    }

    void OnStop(ara::exec::RuntimeContext& context) override {
        context.Log().Info(Name(), "Hello world app stopped");
    }
};

} // namespace

std::unique_ptr<ara::exec::Application> CreateHelloWorldApp() {
    return std::make_unique<HelloWorldApp>();
}

} // namespace openaa::examples::hello_world
