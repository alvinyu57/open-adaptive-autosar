# `ara/exec`

AUTOSAR Adaptive `ara::exec` API surface and a small in-process runtime stub for development.

## Responsibilities

The `ara/exec` package defines execution-management-facing types built on top of `ara/core`.

## Main Building Blocks

- `include/ara/exec/exec_error_domain.h`: error domain and error codes defined for execution management
- `include/ara/exec/execution_client.h`: process-facing execution state reporting API
- `include/ara/exec/function_group.h`: function-group identifier type
- `include/ara/exec/function_group_state.h`: function-group state identifier type
- `include/ara/exec/state_client.h`: function-group state transition API

## Software Architecture Requirements

### Role in the system

- `ara/exec` shall consume the standardized contracts from `ara/core`.
- `ara/exec` shall remain independent from middleware transport implementation details so it can orchestrate different application types uniformly.

### Execution state reporting

- `ExecutionClient` shall provide process-facing reporting of `ExecutionState`.
- `ExecutionClient::Create()` shall validate construction-time prerequisites and report failures through `ara::core::Result`.

### Function-group transitions

- `FunctionGroup` and `FunctionGroupState` shall model AUTOSAR execution-management state identifiers.
- `StateClient::SetState()` shall return an asynchronous result object.
- `StateClient::GetExecutionError()` shall expose the last unexpected termination state for an undefined function-group state when available.

### Quality attributes and evolution

- The module should remain a thin standards-oriented layer on top of `ara/core`.
- The module should be testable with lightweight unit and smoke coverage using stub runtime state.
- Future evolution of `ara/exec` may enrich the process interaction with a real platform execution manager while preserving the AUTOSAR API surface.

## Current Limitations

- The current implementation is an in-process stub and does not communicate with a platform execution manager daemon.
- Function-group transitions complete immediately and are intended only for development and CI validation.
