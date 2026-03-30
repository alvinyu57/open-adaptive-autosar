#pragma once

#include <filesystem>

#include "ara/exec/application_manifest.hpp"

namespace ara::exec {

ApplicationManifest LoadApplicationManifestFromFile(const std::filesystem::path& path);

} // namespace ara::exec
