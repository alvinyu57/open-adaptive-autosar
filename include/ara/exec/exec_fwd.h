#pragma once

#include <cstdint>

#include "ara/core/error_domain.h"

namespace ara::exec {

enum class ExecutionState : std::uint32_t;
enum class ExecErrc : ara::core::ErrorDomain::CodeType;

class ExecErrorDomain;
class ExecException;
class ExecutionClient;
class FunctionGroup;
class FunctionGroupState;
class StateClient;

using ExecutionError = std::uint32_t;

struct ExecutionErrorEvent;

} // namespace ara::exec
