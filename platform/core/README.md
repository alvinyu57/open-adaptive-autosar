# `platform/core`

OpenAA concrete implementation of the `ara/core` interfaces.

## Responsibilities

This module provides the runtime behavior behind the current core abstractions:

- timestamped logging to an injected output stream
- in-process service registration with duplicate detection
- lifecycle state enforcement for applications

## Main Building Blocks

- `openaa/core/application.hpp`: OpenAA concrete `Logger`, `ServiceRegistry`, and `Application` types
- `src/application.cpp`: lifecycle, logging, and registry implementation

## Notes

- This module implements the contracts declared in `ara/core`.
- The current MVP keeps all behavior in-process and synchronous.
- The logger currently emits local timestamps with sub-second precision determined by the implementation.
