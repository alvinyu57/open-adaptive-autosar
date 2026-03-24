# Smoke Tests

This directory contains standalone shell entry points for smoke tests.

## Core Smoke Test

Build the project with tests enabled:

```bash
./build.sh --build-tests
```

Run the smoke test from anywhere:

```bash
/home/alvin/coding/open-adaptive-autosar/tests/smoke/run_core_smoke_test.sh
```

The script resolves the repository root from its own location, so it does not depend on the current working directory.
