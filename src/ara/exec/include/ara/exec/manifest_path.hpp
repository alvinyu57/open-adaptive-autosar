#pragma once

#include <filesystem>
#include <string_view>

namespace ara::exec {

std::filesystem::path ResolveManifestPath(std::string_view argv0, std::string_view manifest_name);

} // namespace ara::exec
