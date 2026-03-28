#pragma once

#include <memory>

#include "ara/core/application.hpp"

namespace openaa::apps::app_manifest {

std::unique_ptr<ara::core::Application> CreateAppManifestApp();

} // namespace openaa::apps::app_manifest
