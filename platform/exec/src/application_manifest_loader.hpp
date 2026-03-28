#pragma once

#include <filesystem>

#include "ara/exec/application_manifest.hpp"

namespace openaa::exec {

ara::exec::ApplicationManifest LoadApplicationManifestFromFile(const std::filesystem::path& path);

} // namespace openaa::exec
