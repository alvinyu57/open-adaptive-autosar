# Tests

Comprehensive test suite including unit tests, smoke tests, and integration validation.

## Test Strategy

The project uses a **three-level testing approach**:

1. **Unit Tests**: Validate individual components in isolation
2. **Smoke Tests**: Validate integrated subsystems end-to-end
3. **CI/CD**: Automated validation in GitHub Actions

## Building All Tests

```bash
./scripts/build/build.sh --build-tests
```

This compiles:
- Unit test binaries in `build/Release/tests/`
- Test data and fixtures
- Integration test harnesses

## Test Organization

### Unit Tests (`unit/`)

**Purpose**: Fast, focused validation of individual modules

**Contents**:
- `core_*_test.cpp`: ara::core module contracts
- `com_*_test.cpp`: ara::com communication APIs
- `initialization_test.cpp`: Runtime initialization
- `logger_test.cpp`: Logging interface
- `execution_manager_manifest_test.cpp`: Manifest loading

**Run**:
```bash
./scripts/test/run_unit_test.sh
```

Details: See [unit/README.md](unit/README.md)

### Smoke Tests (`smoke/`)

**Purpose**: Integration validation and realistic scenarios

**Contents**:
- `core_smoke_test.cpp`: End-to-end runtime behavior

**Run**:
```bash
./scripts/test/run_smoke_test.sh
```

Details: See [smoke/README.md](smoke/README.md)

## Running Tests

### All Tests

Run both unit and smoke tests:

```bash
./scripts/build/build.sh --build-tests
./scripts/test/run_unit_test.sh
./scripts/test/run_smoke_test.sh
```

### Specific Test Suite

Unit tests only:
```bash
./scripts/test/run_unit_test.sh
```

Smoke tests only:
```bash
./scripts/test/run_smoke_test.sh
```

### With CTest

List all tests:
```bash
cd build/Release
ctest --list-tests
```

Run tests with filter:
```bash
ctest -R "core_application" -VV
```

Run with verbose output:
```bash
ctest --output-on-failure -VV
```

## Test Coverage Summary

| Component | Unit Tests | Smoke Tests | Integration |
|-----------|-----------|------------|-------------|
| ara::core | 8+ tests | ✓ | ✓ |
| ara::exec | 2+ tests | ✓ | ✓ |
| ara::com  | 3+ tests | ✓ | ✓ |
| ara::log  | 1+ test  | ✓ | ✓ |
| Manifests | 1+ test  | ✓ | ✓ |
| **Total** | **31+ tests** | **✓** | **✓** |

## CI/CD Pipeline

Tests run automatically on every PR:

```yaml
- Unit Tests: ctest --label-regex unit
- Smoke Tests: ctest --label-regex smoke
- Linting: clang-format, clang-tidy
```

**Merge Requirements**:
- ✅ All tests pass
- ✅ No clang-tidy warnings
- ✅ Code formatted with clang-format

## Test Data and Fixtures

### Location

Test data is in:
- `apps/02_tire_pressure/data/` - Tire pressure samples (JSON)
- `src/manifests/` - AUTOSAR manifests (ARXML)

### Usage

Applications and tests reference data files, supporting:
- Realistic sensor data simulation
- Manifest-driven configuration
- Integration scenario validation

## Performance Benchmarks

| Test Suite | Time  | Platform |
|-----------|-------|----------|
| Unit Tests | <1s | Linux x86_64 |
| Smoke Tests | <2s | Linux x86_64 |
| Full Build | ~10-15s | Linux x86_64 |

## Troubleshooting

### Test Compilation Issues

```bash
# Clean rebuild
rm -rf build/
./scripts/build/build.sh --build-tests
```

### Test Execution Failure

```bash
# Run with detailed output
./scripts/test/run_unit_test.sh --verbose
ctest -VV --output-on-failure
```

### Flaky Tests

Document in:
- Test comments with known issues
- GitHub Issues for cross-platform differences

## Creating New Tests

### Adding a Unit Test

1. Create test file in `tests/unit/my_module_test.cpp`
2. Include `#include <gtest/gtest.h>`
3. Add test to `tests/unit/CMakeLists.txt`
4. Rebuild: `./scripts/build/build.sh --build-tests`

Example:
```cpp
#include <gtest/gtest.h>
#include "ara/core/initialization.h"

TEST(MyTest, ShouldPass) {
    EXPECT_TRUE(true);
}
```

### Adding a Smoke Test

1. Extend `tests/smoke/core_smoke_test.cpp`
2. Add test scenario to existing test harness
3. Rebuild: `./scripts/build/build.sh --build-tests`

## Resources

- [GoogleTest Documentation](https://google.github.io/googletest/)
- [Unit Tests Guide](unit/README.md)
- [Smoke Tests Guide](smoke/README.md)
- [AUTOSAR Testing Best Practices](https://www.autosar.org/)
