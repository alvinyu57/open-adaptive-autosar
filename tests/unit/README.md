# Unit Tests

Comprehensive unit test suite for Adaptive AUTOSAR runtime modules.

## Overview

Unit tests provide isolated validation of core functionality and error handling for all major components.

## Test Organization

Tests are organized by module:

```
tests/unit/
├── CMakeLists.txt
├── abort_test.cpp                    # ara::core::AddAbortHandler
├── core_application_test.cpp         # ara::core::Application lifecycle
├── core_error_domain_test.cpp        # ara::core::ErrorDomain contracts
├── com_error_domain_test.cpp         # ara::com::ComErrorDomain
├── com_runtime_test.cpp              # ara::com service registration and discovery
├── com_service_types_test.cpp        # ara::com type validation
├── execution_manager_manifest_test.cpp  # Manifest loading and parsing
├── initialization_test.cpp           # ara::core::Initialize/Deinitialize
└── logger_test.cpp                   # ara::log::Logger
```

## Building Tests

Build all test targets:

```bash
./scripts/build/build.sh --build-tests
```

Test binaries are output to `build/Release/tests/`.

## Running Tests

### All Unit Tests

Run the complete unit test suite:

```bash
./scripts/test/run_unit_test.sh
```

This executes:
```bash
./build/Release/tests/openaa_core_unit_test
```

### Individual Tests (With CTest)

List available tests:

```bash
cd build/Release
ctest --label-regex unit --list-tests
```

Run a specific test:

```bash
ctest --label-regex unit -R "core_error_domain" -VV
```

Run with verbose output:

```bash
ctest --label-regex unit -VV
```

## Test Coverage

### Core Module (8 tests)

**`core_error_domain_test.cpp`**:
- Error domain initialization and identity
- Error message mapping for all CoreErrc codes
- Exception throwing behavior
- Message string consistency

**`core_application_test.cpp`**:
- Application creation and naming
- Lifecycle state transitions
- Initialization and execution flow
- Resource cleanup on shutdown

**`initialization_test.cpp`**:
- Framework initialization with argc/argv
- Framework deinitialization
- Multiple initialization/cleanup cycles
- Proper state management

**`abort_test.cpp`**:
- Handler registration and invocation
- Duplicate handler prevention
- Handler signature validation
- Abort reporting

### Communication Module (3 tests)

**`com_error_domain_test.cpp`**:
- ComErrorDomain identity and name
- Error code message mapping
- Exception creation and handling

**`com_runtime_test.cpp`**:
- Service registration and deregistration
- Instance mapping
- Service discovery and lookup
- Binding metadata management

**`com_service_types_test.cpp`**:
- ServiceIdentifierType type safety
- InstanceIdentifier validation
- Type serialization and comparison

### Logging Module (1 test)

**`logger_test.cpp`**:
- Logger creation with component names
- All severity levels (Info, Warn, Error)
- Log message formatting
- Stream-based output capture

### Execution & Manifest (1 test)

**`execution_manager_manifest_test.cpp`**:
- Manifest JSON parsing
- Application manifest loading
- Service declaration extraction
- Automatic startup configuration

## Test Framework

Tests use **GoogleTest (gtest)** for:
- Test fixtures and parameterized tests
- Assertion macros (ASSERT_*, EXPECT_*)
- Test discovery and reporting
- Integration with CTest

## Writing New Tests

### Template

```cpp
#include <gtest/gtest.h>
#include "ara/core/initialization.h"

class MyModuleTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize test fixtures
    }

    void TearDown() override {
        // Clean up after test
    }
};

TEST_F(MyModuleTest, ShouldDoSomething) {
    // Arrange
    int expected = 42;
    
    // Act
    int actual = MyFunction();
    
    // Assert
    EXPECT_EQ(expected, actual);
}
```

### Guidelines

- **One concern per test**: Each test validates a single behavior
- **Clear naming**: Use descriptive test names (e.g., `ShouldThrowOnInvalidInput`)
- **Minimal setup**: Use test fixtures only for shared state
- **Fast execution**: Avoid sleep, network, filesystem where possible
- **Deterministic**: Tests should produce consistent results regardless of order
- **Independent**: Tests should not depend on execution order or shared state

## CI Integration

Tests are run in GitHub Actions:

```bash
ctest --label-regex unit --output-on-failure
```

Any test failure blocks PR merges.

## Performance Characteristics

- **Suite Size**: 31+ tests across all modules
- **Typical Runtime**: <1 second total execution
- **Coverage**: Critical paths in core, communication, and logging modules

## Troubleshooting

### Test Fails With "Handler Already Registered"

The abort handler test uses static state. Run tests in isolation:

```bash
ctest --label-regex "abort_test" -VV
```

### Test Fails With Segmentation Fault

Ensure:
1. `ara::core::Initialize()` is called in SetUp
2. `ara::core::Deinitialize()` is called in TearDown
3. No dangling pointers or use-after-free

### Test Fails With Timeout

Check for:
1. Infinite loops or blocking I/O
2. Missing mock implementations
3. Race conditions in concurrent code

## Future Test Enhancements

- Add parameterized tests for error codes and edge cases
- Implement chaos engineering scenarios (service failures, timeouts)
- Add performance benchmarks for hot paths
- Expand coverage to out-of-process scenarios
