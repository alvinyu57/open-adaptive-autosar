#include "openaa/examples/hello_world/hello_world_app.hpp"

#include <sstream>

namespace openaa::examples::hello_world {

namespace {

class HelloWorldApp final : public openaa::core::Application {
  public:
    HelloWorldApp() : openaa::core::Application("examples.hello_world") {}

  private:
    void OnInitialize(openaa::core::RuntimeContext &context) override {
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

    void OnStart(openaa::core::RuntimeContext &context) override {
        context.Log().Info(Name(), "Offer greeting: Hello from Adaptive AUTOSAR MVP");
    }

    void OnStop(openaa::core::RuntimeContext &context) override {
        context.Log().Info(Name(), "Hello world app stopped");
    }
};

} // namespace

std::unique_ptr<openaa::core::Application> CreateHelloWorldApp() {
    return std::make_unique<HelloWorldApp>();
}

} // namespace openaa::examples::hello_world
