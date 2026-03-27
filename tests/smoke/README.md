# Smoke Tests

Smoke-test entry points now live under `scripts/test`.

## Core Smoke Test

Build the project with tests enabled:

```bash
./scripts/build/build.sh --build-tests
```

Run the smoke test from anywhere:

```bash
/home/alvin/coding/open-adaptive-autosar/scripts/test/run_smoke_test.sh
```

The script resolves the repository root from its own location, so it does not depend on the current working directory.
