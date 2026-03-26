# `ara/core`

Standardized ARA-facing core interfaces used by the OpenAA platform layer.

## Responsibilities

The `ara/core` package defines the smallest reusable interfaces needed by adaptive applications and higher-level runtime managers:

- application lifecycle state handling
- runtime logging
- in-process service registration
- a shared runtime context passed into applications

This layer intentionally stays transport-agnostic and implementation-agnostic. Concrete lifecycle handling, logging, and service registry behavior belong in the platform layer.

## Main Building Blocks

- `Logger`: interface for runtime logging.
- `ServiceRegistry`: interface for service registration and discovery snapshots.
- `RuntimeContext`: lightweight bundle of shared runtime service references.
- `Application`: application lifecycle contract exposed to platform orchestration.

## Software Architecture Requirements

### Scope and layering

- `core` shall define reusable runtime abstractions that can be consumed by `exec`, examples, and future middleware modules.
- `core` shall not depend on execution-policy concerns such as process supervision, manifest loading, restart strategy, or scheduling policy.
- `core` shall keep its public interfaces independent from transport-specific protocols such as SOME/IP or ara::com bindings.

### Application lifecycle

- `Application` shall model lifecycle progression with explicit states: `kCreated`, `kInitialized`, `kRunning`, and `kStopped`.
- `Application::Initialize()` shall only be accepted from `kCreated`; any other source state shall be rejected with a logic error.
- `Application::Run()` shall only be accepted from `kInitialized`; any other source state shall be rejected with a logic error.
- `Application::Stop()` shall be idempotent for applications that are already stopped or were never initialized.
- `Application` shall expose lifecycle extension points through `OnInitialize()`, `OnStart()`, and `OnStop()` so module users can provide application-specific behavior without reimplementing state handling.

### Shared runtime context

- `RuntimeContext` shall provide applications access to shared runtime capabilities through stable references rather than ownership transfer.
- `RuntimeContext` shall expose at least logging and service-registration facilities to application code.
- `RuntimeContext` shall remain lightweight so it can be created by the execution layer for each runtime session without additional orchestration state.

### Service registration

- `ServiceRegistry` shall store service entries identified by a unique `service_id`.
- `ServiceRegistry::Register()` shall reject duplicate `service_id` values by returning `false` and preserving the existing entry.
- `ServiceRegistry::List()` shall return a snapshot of the currently registered services suitable for status reporting and tests.
- `ServiceRegistry` shall protect internal state against concurrent access when registering or listing services.

### Logging

- `Logger` shall provide at least `Info`, `Warn`, and `Error` severity levels.
- Log messages emitted by `Logger` shall include a timestamp, severity, and component identifier so runtime activity can be correlated across modules.
- `Logger` shall write to an injected output stream so tests or embedding environments can redirect log output without changing the core API.

### Quality attributes

- `core` should remain small, deterministic, and easy to unit-test without external infrastructure.
- Public interfaces in `core` should favor standard C++ types and ownership semantics that make misuse visible at compile time or fail fast at runtime.
- Architecture changes in `core` should preserve backward compatibility for existing `exec` integration unless the public API is intentionally revised.

## Current Limitations

- Service registration is currently in-process only.
- Lifecycle execution is synchronous.
- The module does not yet model manifests, health management, or state management beyond the local application state machine.
