# `ara/core`

Core runtime APIs and their current in-process implementation.

## Responsibilities

The `ara/core` package defines the smallest reusable runtime building blocks needed by adaptive applications and higher-level runtime managers:

- application lifecycle contracts
- runtime logging contracts
- service registration contracts
- a shared runtime context passed into applications

This layer intentionally stays transport-agnostic. The current repository keeps the default lifecycle, logging, and service-registry implementation in the same package so applications only depend on `ara/core`.

## Main Building Blocks

- `ara/core/application.hpp`: `Logger`, `ServiceRegistry`, `RuntimeContext`, `Application`, and supporting types

## Software Architecture Requirements

### Scope and layering

- `core` shall define reusable runtime abstractions that can be consumed by `exec`, examples, and future middleware modules.
- `core` shall not depend on execution-policy concerns such as process supervision, manifest loading, restart strategy, or scheduling policy.
- `core` shall keep its public interfaces independent from transport-specific protocols such as SOME/IP or ara::com bindings.

### Application lifecycle

- `Application` shall expose explicit lifecycle operations for initialization, execution, and shutdown.
- `Application` shall expose observable lifecycle state through `State()`.
- `Application` shall expose a stable application identifier through `Name()`.
- `ara/core` shall keep lifecycle enforcement simple and predictable for in-process execution.

### Shared runtime context

- `RuntimeContext` shall provide applications access to shared runtime capabilities through stable references rather than ownership transfer.
- `RuntimeContext` shall expose at least logging and service-registration facilities to application code.
- `RuntimeContext` shall remain lightweight so it can be created by an execution layer without additional orchestration state.

### Service registration

- `ServiceRegistry` shall define registration of service entries identified by a `service_id`.
- `ServiceRegistry::Register()` shall report whether registration succeeded.
- `ServiceRegistry::List()` shall return a snapshot suitable for status reporting, diagnostics, or tests.
- `ara/core` shall not prescribe the concrete storage, synchronization, or duplicate-handling strategy beyond the observable contract.

### Logging

- `Logger` shall provide at least `Info`, `Warn`, and `Error` severity levels.
- `Logger` shall accept a component identifier and message payload for each log record.
- `ara/core` shall not require a specific timestamp format, sink, or buffering strategy.

### Quality attributes

- `ara/core` should remain small, deterministic, and easy to consume without external infrastructure.
- Public interfaces in `ara/core` should favor standard C++ types and clear ownership semantics.
- Architecture changes in `ara/core` should preserve compatibility for dependent execution and application code unless the public API is intentionally revised.

## Current Limitations

- The interface set is intentionally minimal and does not yet cover manifests, health management, or richer state-management services.
