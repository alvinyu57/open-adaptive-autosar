# `platform`

Concrete OpenAA runtime implementations behind the standardized `ara` interfaces.

## Purpose

The `platform` layer contains the executable behavior of the current MVP runtime. It is where logging, service registration, lifecycle state handling, and execution orchestration are implemented.

In the current layout, this layer contains:

- `platform/core`: OpenAA implementation of the `ara/core` contracts
- `platform/exec`: OpenAA implementation of the `ara/exec` contracts

## Design Intent

- `platform` may choose internal data structures, synchronization, and logging behavior as long as it satisfies the `ara` contracts.
- `platform` is the correct place for OpenAA-specific runtime policy.
- `platform` should stay replaceable from the point of view of code that only depends on `ara`.
