#pragma once

#include "ara/core/result.h"

namespace ara::core {

Result<void> Initialize() noexcept;
Result<void> Initialize(int argc, char** argv) noexcept;
Result<void> Deinitialize() noexcept;

} // namespace ara::core
