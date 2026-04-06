# `ara`

Adaptive AUTOSAR APIs and their current in-process implementation.

## Purpose

The `src/ara` tree is now the single home for the repository's Adaptive AUTOSAR runtime surface.

In the current MVP, it contains:

- `ara/log`: logging contracts and the default stream-based logger
- `ara/core`: lifecycle, runtime context, and service registry support
- `ara/exec`: execution management, manifest loading, and demo runner support

## Design Intent

- `src/ara` should stay small, readable, and easy to extend.
- Public headers under `ara/...` should remain the primary integration surface for apps and tests.
- Runtime implementation details should live beside the corresponding `ara` package instead of a separate `platform` tree.
