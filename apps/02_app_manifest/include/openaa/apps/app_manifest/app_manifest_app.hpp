#pragma once

#include <memory>

#include "ara/exec/application.hpp"

namespace openaa::apps::app_manifest {

std::unique_ptr<ara::exec::Application> CreateAppManifestApp();

} // namespace openaa::apps::app_manifest
