# `ara/log`

Logging APIs and their current in-process implementation.

## Responsibilities

The `ara/log` package defines the repository's logging interface and the default stream-based logger implementation used by the MVP runtime.

## Main Building Blocks

- `include/ara/log/logger.hpp`: `Logger` with `Info`, `Warn`, and `Error` severity-level operations

## Software Architecture Requirements

### Role in the system

- `ara/log` shall provide reusable logging contracts consumable by `ara/core`, `ara/exec`, examples, and tests.
- `ara/log` shall remain independent from execution policy, manifest loading, and service-registry concerns.

### Logging behavior

- `Logger` shall provide at least `Info`, `Warn`, and `Error` severity levels.
- `Logger` shall accept a component identifier and message payload for each log record.
- `ara/log` shall not require a specific sink beyond the observable logger contract.

## Current Limitations

- The current implementation is intentionally simple and stream-based, without structured payloads, filtering, or multiple backends.
