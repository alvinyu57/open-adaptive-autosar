# `ara/exec`

Standardized execution-management interfaces exposed above the platform layer.

## Responsibilities

The `ara/exec` package defines orchestration-facing interfaces that operate on the abstractions provided by `ara/core`.

In the current MVP it is responsible for:

- registering application instances with the runtime
- creating a shared runtime context
- driving application initialization and start-up in order
- driving shutdown in reverse order
- exposing a runtime service snapshot for observability and tests

This layer does not implement startup policy itself. Concrete execution-manager behavior belongs in `platform/exec`.

## Main Building Blocks

- `ExecutionManager`: interface for lifecycle orchestration of adaptive applications.

## Software Architecture Requirements

### Role in the system

- `ara/exec` shall define orchestration interfaces that can be implemented by one or more platform runtimes.
- `ara/exec` shall consume the standardized contracts from `ara/core` rather than concrete platform types.
- `ara/exec` shall remain independent from middleware transport implementation details so it can orchestrate different application types uniformly.

### Application registration and ownership

- `ExecutionManager` shall accept ownership of applications through `std::unique_ptr<ara::core::Application>`.
- `ExecutionManager::AddApplication()` shall allow platform implementations to register application instances before startup.
- `ara/exec` shall not mandate a specific ownership container or logging strategy for the underlying platform implementation.

### Startup behavior

- `ExecutionManager::Start()` shall create a runtime context that shares one service registry and one logger across all managed applications in the same runtime session.
- `ExecutionManager::Start()` shall initialize each registered application before starting it.
- `ExecutionManager::Start()` shall process applications in registration order so startup behavior remains deterministic.
- After startup, the execution layer shall publish a summary of the runtime service inventory for diagnostics and smoke testing.

### Shutdown behavior

- `ExecutionManager::Stop()` shall invoke application shutdown in reverse registration order to reduce teardown dependency risk.
- `ExecutionManager::Stop()` shall reuse the same shared runtime facilities model during shutdown so applications can log and release services consistently.
- The execution layer shall log shutdown progress for each application and emit a final runtime-stopped message.

### Shared runtime data

- `ExecutionManager` shall own the runtime `ServiceRegistry` instance for the lifetime of the managed runtime session.
- `ExecutionManager::Services()` shall expose read-only access to the registry so tests and diagnostics can inspect registered services without mutating manager state.
- All managed applications in a single execution manager instance shall observe the same service registry contents through the shared runtime context.

### Quality attributes and evolution

- The MVP execution path should remain synchronous and easy to understand until manifest-driven launching or multi-process supervision is introduced.
- The module should be testable with lightweight unit and smoke coverage using stub applications.
- Future evolution of `exec` may add manifests, health supervision, restart handling, and process isolation, but those concerns should extend rather than invalidate the current core lifecycle contract.

## Current Limitations

- Applications are instantiated directly in code rather than loaded from manifests.
- Startup failure handling and rollback policy are not yet modeled.
- Runtime supervision, restart, and separate process execution are not yet implemented.
