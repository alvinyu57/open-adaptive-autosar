#pragma once

#include "ara/exec/exec_fwd.h"
#include "ara/exec/function_group.h"

namespace ara::exec {

struct ExecutionErrorEvent final {
    ExecutionError executionError{};
    FunctionGroup functionGroup;
};

} // namespace ara::exec
