#include "ara/exec/manifest_path.hpp"

namespace ara::exec {

std::filesystem::path ResolveManifestPath(
    std::string_view argv0,
    std::string_view manifest_name) {
    return std::filesystem::path(argv0).parent_path() / "manifests" / manifest_name;
}

} // namespace ara::exec
