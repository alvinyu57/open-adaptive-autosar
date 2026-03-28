#include "openaa/apps/app_manifest/app_manifest_app.hpp"

#include "openaa/core/application.hpp"

namespace openaa::apps::app_manifest {

namespace {

class AppManifestApp final : public openaa::core::Application {
public:
    AppManifestApp()
        : openaa::core::Application("apps.02_app_manifest.demo") {}

private:
    void OnInitialize(ara::core::RuntimeContext& context) override {
        const bool inserted = context.Services().Register({
            .service_id = "apps.02.app_manifest.greeter",
            .endpoint = "local://app-manifest-greeter",
            .owner = Name(),
        });

        if (!inserted) {
            context.Log().Warn(Name(), "App manifest greeter service already registered");
            return;
        }

        context.Log().Info(Name(), "Registered app manifest greeter service");
    }

    void OnStart(ara::core::RuntimeContext& context) override {
        context.Log().Info(Name(), "Offer greeting: manifest-driven startup is active");
    }

    void OnStop(ara::core::RuntimeContext& context) override {
        context.Log().Info(Name(), "App manifest demo stopped");
    }
};

} // namespace

std::unique_ptr<ara::core::Application> CreateAppManifestApp() {
    return std::make_unique<AppManifestApp>();
}

} // namespace openaa::apps::app_manifest
