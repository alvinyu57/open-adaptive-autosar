# Smoke Tests

Integration and basic functionality tests for core Adaptive AUTOSAR runtime components.

## Overview

Smoke tests validate end-to-end runtime behavior in simplified scenarios, complementing unit tests with integration coverage.

## Core Smoke Test Coverage

The smoke test suite includes:

- **Application Lifecycle**: initialization, running state, clean shutdown
- **Service Registry**: service registration, lookup, and listing
- **Execution Management**: function-group state transitions and error reporting
- **Manifest Loading**: ARXML-based application manifest processing

## Running Smoke Tests

Build the project with test targets enabled:

```bash
./scripts/build/build.sh --build-tests
```

Run the smoke test:

```bash
./scripts/test/run_smoke_test.sh
```

The script automatically:
- Resolves the repository root from its own location
- Locates the test binary in the build directory
- Reports pass/fail status and any assertion failures

## Test Script Locations

- Entry point: `scripts/test/run_smoke_test.sh`
- Test source: `tests/smoke/core_smoke_test.cpp`
- Build output: `build/Release/tests/smoke/openaa_core_smoke_test`

## Design Philosophy

Smoke tests are **not** unit tests:
- They exercise **integrated subsystems** rather than isolated components
- They validate **observable behavior** from an application perspective
- They catch **integration regressions** that unit tests cannot detect
- They provide **confidence** for release readiness before deployment

## Integration with CI

The smoke test is run as part of the GitHub Actions CI pipeline:

```bash
ctest --label-regex smoke
```

Failure in smoke tests blocks PR merges and requires investigation before integration.
