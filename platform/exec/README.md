# `platform/exec`

OpenAA concrete implementation of the `ara/exec` interfaces.

## Responsibilities

This module provides the current MVP execution runtime:

- owning registered applications
- creating a shared runtime context
- initializing and starting applications in registration order
- stopping applications in reverse order
- exposing registered services for diagnostics and tests

## Main Building Blocks

- `openaa/exec/execution_manager.hpp`: OpenAA concrete execution manager
- `src/execution_manager.cpp`: orchestration logic
- `src/main.cpp`: small runnable demo wired to the concrete platform runtime

## Notes

- This module implements the contracts declared in `ara/exec`.
- The current runtime is intentionally synchronous and in-process.
- Future extensions such as manifests, supervision, or process isolation should build on this layer rather than leaking into `ara`.
