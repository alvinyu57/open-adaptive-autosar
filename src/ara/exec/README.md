# `ara/exec`

Execution-management APIs and their current in-process implementation.

## Responsibilities

The `ara/exec` package defines orchestration-facing types built on top of `ara/core` and also contains the repository's current execution-manager implementation.

## Main Building Blocks

- `ara/exec/execution_manager.hpp`: `ExecutionManager` for lifecycle orchestration of adaptive applications
- `ara/exec/application_manifest.hpp`: manifest data model for Adaptive AUTOSAR-style application startup metadata
- `ara/exec/manifest_path.hpp`: helper for locating co-deployed manifests beside executables

## Software Architecture Requirements

### Role in the system

- `ara/exec` shall consume the standardized contracts from `ara/core`.
- `ara/exec` shall remain independent from middleware transport implementation details so it can orchestrate different application types uniformly.

### Application registration and ownership

- `ExecutionManager` shall accept ownership of applications through `std::unique_ptr<ara::core::Application>`.
- `ExecutionManager::AddApplication()` shall allow runtimes to register application instances before startup.
- `ExecutionManager::RegisterApplicationFactory()` shall allow manifest-driven runtimes to bind application identifiers to concrete application factories.
- `ExecutionManager::LoadApplicationManifest()` shall allow runtimes to load application startup metadata before execution begins.

### Startup behavior

- `ExecutionManager::Start()` shall expose an explicit entry point for beginning managed application execution.
- `ara/exec` shall not prescribe startup ordering, logging detail, or context construction beyond what is visible through the interface contract.

### Shutdown behavior

- `ExecutionManager::Stop()` shall expose an explicit entry point for stopping managed application execution.
- `ara/exec` shall not prescribe shutdown ordering, rollback behavior, or supervision policy beyond what is visible through the interface contract.

### Shared runtime data

- `ExecutionManager::Services()` shall expose read-only access to a service registry view suitable for diagnostics and tests.
- `ara/exec` shall not require a particular ownership model for the underlying registry implementation.

### Quality attributes and evolution

- The module should remain a thin orchestration layer on top of `ara/core`.
- The module should be testable with lightweight unit and smoke coverage using stub applications.
- Future evolution of `ara/exec` may add richer execution-management contracts, but those additions should preserve a clear separation from platform-specific policy.

## Current Limitations

- Restart policy, health supervision, and process isolation are not yet represented in the interface layer.
