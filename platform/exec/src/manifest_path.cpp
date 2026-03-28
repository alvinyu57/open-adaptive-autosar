#include "openaa/exec/manifest_path.hpp"

namespace openaa::exec {

std::filesystem::path ResolveManifestPath(
    std::string_view argv0,
    std::string_view manifest_name) {
    return std::filesystem::path(argv0).parent_path() / "manifests" / manifest_name;
}

} // namespace openaa::exec
