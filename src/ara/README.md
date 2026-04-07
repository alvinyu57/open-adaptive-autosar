# `ara`

Adaptive AUTOSAR APIs and their in-process reference implementation.

## Purpose

The `src/ara` tree is the single home for the repository's Adaptive AUTOSAR runtime API surface and implementation.

The current implementation includes:

- **`ara/log`**: Logging contracts and stream-based logger implementation
- **`ara/core`**: Lifecycle management, runtime context, and service registry
- **`ara/exec`**: Execution management API and function-group state control
- **`ara/com`**: Communication contracts and service binding runtime

## Design Intent

- **Modularity**: Each `ara/*` namespace provides clear, focused contracts
- **Simplicity**: Implementation favors clarity over feature completeness
- **Extensibility**: Public headers remain the primary integration surface
- **Transport Independence**: Communication APIs remain agnostic to binding technologies (SOME/IP, native IPC, etc.)

## Module Overview

### `ara/core`
Core abstractions needed by all adaptive applications:
- Application lifecycle (initialization, running, shutdown)
- Shared runtime context accessible to all components
- Service registry for local service discovery
- Error domains and exception handling

### `ara/exec`
Execution management for process and function-group orchestration:
- Function-group state machine and transitions
- Process execution reporting and error handling
- Manifest-driven execution policies

### `ara/com`
Communication service contracts and runtime bindings:
- Service identifiers and instance mapping
- Binding metadata for transport selection
- Service discovery and instance resolution

### `ara/log`
Logging interface for runtime diagnostics:
- Severity levels (Info, Warn, Error)
- Component identifiers for log filtering
- Stream-based implementation for lightweight scenarios
