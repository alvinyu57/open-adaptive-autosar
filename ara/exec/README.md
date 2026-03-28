# `ara/exec`

Standardized execution-management interfaces exposed above the platform layer.

## Responsibilities

The `ara/exec` package defines orchestration-facing interfaces that operate on the abstractions provided by `ara/core`.

This layer does not implement startup policy itself. Concrete execution-manager behavior belongs in `platform/exec`.

## Main Building Blocks

- `ara/exec/execution_manager.hpp`: `ExecutionManager` interface for lifecycle orchestration of adaptive applications
- `ara/exec/application_manifest.hpp`: manifest data model for Adaptive AUTOSAR-style application startup metadata

## Software Architecture Requirements

### Role in the system

- `ara/exec` shall define orchestration interfaces that can be implemented by one or more platform runtimes.
- `ara/exec` shall consume the standardized contracts from `ara/core` rather than concrete platform types.
- `ara/exec` shall remain independent from middleware transport implementation details so it can orchestrate different application types uniformly.

### Application registration and ownership

- `ExecutionManager` shall accept ownership of applications through `std::unique_ptr<ara::core::Application>`.
- `ExecutionManager::AddApplication()` shall allow platform implementations to register application instances before startup.
- `ExecutionManager::RegisterApplicationFactory()` shall allow manifest-driven runtimes to bind application identifiers to concrete application factories.
- `ExecutionManager::LoadApplicationManifest()` shall allow platform runtimes to load application startup metadata before execution begins.
- `ara/exec` shall not mandate a specific ownership container or logging strategy for the underlying platform implementation.

### Startup behavior

- `ExecutionManager::Start()` shall expose an explicit platform entry point for beginning managed application execution.
- `ara/exec` shall not prescribe startup ordering, logging detail, or context construction beyond what is visible through the interface contract.

### Shutdown behavior

- `ExecutionManager::Stop()` shall expose an explicit platform entry point for stopping managed application execution.
- `ara/exec` shall not prescribe shutdown ordering, rollback behavior, or supervision policy beyond what is visible through the interface contract.

### Shared runtime data

- `ExecutionManager::Services()` shall expose read-only access to a service registry view suitable for diagnostics and tests.
- `ara/exec` shall not require a particular ownership model for the underlying registry implementation.

### Quality attributes and evolution

- `ara/exec` should remain a thin orchestration contract layered on top of `ara/core`.
- The module should be testable with lightweight unit and smoke coverage using stub applications and platform implementations.
- Future evolution of `ara/exec` may add richer execution-management contracts, but those additions should preserve a clear separation from platform-specific policy.

## Current Limitations

- The current repository provides one concrete in-process implementation in `platform/exec`.
- Restart policy, health supervision, and process isolation are not yet represented in the interface layer.
