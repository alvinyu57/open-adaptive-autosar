# `ara`

Standardized Adaptive AUTOSAR-facing interfaces exposed by this repository.

## Purpose

The `ara` layer defines API contracts that application code and platform implementations can depend on without coupling directly to OpenAA runtime internals.

In the current MVP, this layer contains:

- `ara/core`: core lifecycle and runtime service interfaces
- `ara/exec`: execution-management interfaces built on top of `ara/core`

## Design Intent

- `ara` should remain implementation-agnostic.
- `ara` should not embed OpenAA-specific runtime policy or storage details.
- `ara` headers should be stable contracts consumed by code in `platform`, `examples`, and future modules.

## Relationship To `platform`

The `platform` layer provides the concrete OpenAA implementation of these interfaces. In other words:

- `ara/...` answers "what is the contract?"
- `platform/...` answers "how does OpenAA implement it?"
