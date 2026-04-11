# open-adaptive-autosar

Adaptive AUTOSAR open-source platform for research, prototyping, and lightweight automotive middleware development.

## Overview

This project provides a **lightweight and modular** implementation of key concepts from **Adaptive AUTOSAR**, including:

- **Core Runtime**: Application lifecycle, service registry, error domains, and shared runtime context
- **Execution Management**: Process execution, function-group state transitions, and execution manifests
- **Communication Services**: Service-oriented interfaces, instance resolution, and binding metadata
- **Logging**: Severity-level logging with configurable components
- **ARXML Support**: Machine manifests and execution manifests using Adaptive AUTOSAR schema (r54)

### Key Modules

- **`ara::core`**: Minimal reusable runtime abstractions for applications and execution managers
- **`ara::exec`**: AUTOSAR execution-management API surface and state reporting
- **`ara::com`**: Service-oriented communication contracts and runtime binding
- **`ara::log`**: Logging interface with stream-based implementation

> [!NOTE]
> This is **NOT a production-ready AUTOSAR stack**, but a **simplified open platform** for:
> * Learning AUTOSAR Adaptive architecture and best practices
> * Rapid prototyping of Adaptive AUTOSAR applications
> * Academic research into automotive middleware
> * Experimentation with execution concepts and manifest-driven frameworks

## Quick Start

### Prerequisites

- C++20 compiler (GCC 11+ or Clang 14+)
- CMake 3.20+
- Conan 2.0+
- Python 3.8+

### Build and Run

```bash
# Clone the repository
git clone https://github.com/yourusername/open-adaptive-autosar.git
cd open-adaptive-autosar

# Build the project with tests and examples enabled
./scripts/build/build.sh --build-tests --build-apps

# Run unit tests
./scripts/test/run_unit_test.sh

# Run smoke tests
./scripts/test/run_smoke_test.sh

# Run the tire-pressure example (consumer + provider)
./scripts/demo/run_02_tire_presure.sh
```

## Build

### Option 1: Helper Script

```bash
./scripts/build/build.sh
```

Enable examples and tests:

```bash
./scripts/build/build.sh --build-examples --build-tests
```

Build a different configuration:

```bash
./scripts/build/build.sh --build-type Debug --shared
```

Supported options:

- `--build-examples`: enable example targets
- `--build-tests`: enable unit and smoke test targets
- `--build-type <type>`: switch Conan/CMake build type, for example `Debug` or `Release`
- `--conan-option <value>`: pass an extra Conan option and repeat as needed

Build artifacts are generated under `build/<BuildType>`, for example `build/Release`.

### Option 2: Conan Only

```bash
conan profile detect --force
conan install . --output-folder=build --build=missing \
  -s build_type=Release \
  -o '&:build_examples=False' \
  -o '&:build_tests=False'
conan build . --output-folder=build
```

### Option 3: Docker + Conan

Build the container image:

```bash
docker build \
  --build-arg USER_ID="$(id -u)" \
  --build-arg GROUP_ID="$(id -g)" \
  -t openaa-build .
```

Passing the host `UID/GID` avoids file permission problems and also prevents the common `groupadd` failure that happens when `1000:1000` is already occupied inside the base image.

Use the container to install dependencies and compile the project:

```bash
docker run --rm \
  -v "$(pwd):/workspace" \
  -w /workspace \
  openaa-build \
  bash -lc "
    conan profile detect --force && 
    
    conan install . \
      --output-folder=build \
      --build=missing \
      -s build_type=Release \
      -o '&:build_tests=True' \
      &&
    
    conan build . \
      --output-folder=build \
      -o '&:build_tests=True'    
  "
```

This keeps the host machine clean and makes Docker the single source of truth for the build environment.

The mapping is:

- Conan `build_apps` -> CMake `OPEN_AA_BUILD_APPS`
- Conan `build_tests` -> CMake `OPEN_AA_BUILD_TESTS`

## Run

### Execution Manager

The execution manager loads manifests and orchestrates application lifecycle:

```bash
# Load the machine manifest and start configured applications
./build/Release/bin/ara_exec

# Or point to a specific manifest
./build/Release/bin/ara_exec /path/to/machine_manifest.json
```

The manifest provides:
- Default timeouts and function-group policies
- Machine-level configuration and resource constraints
- Startup behavior for registered applications

### Individual Applications

Run the tire-pressure example directly (without the execution manager):

**Provider** (runs indefinitely, serving requests):
```bash
./build/Release/apps/02_tire_pressure/openaa_app_02_tp_provider
```

**Consumer** (requests samples and exits):
```bash
./build/Release/apps/02_tire_pressure/openaa_app_02_tp_consumer
```

Or use the demo script:
```bash
./scripts/demo/run_02_tire_presure.sh
```

### Manifest-Driven Execution

For manifest-based application startup, the execution manager reads:

1. **Machine manifest** (`machine_manifest.json`): Platform topology
2. **Application manifests** (`app_execution_manifest.json`): Per-app configuration
3. **Merged manifest** (optional): Combined platform + application metadata

This enables:
- **Declarative startup**: No hardcoded application lists in code
- **Dynamic configuration**: Change startup behavior via manifest edits
- **Standardized integration**: AUTOSAR-compliant tooling and deployment

## Manifests

The project uses **AUTOSAR ARXML manifests** (XML format) to declare:
- Machine configuration, function groups, and startup policies
- Per-process execution details, service offerings, and state dependencies
- Adaptive runtime behavior in a standardized, declarative way

### Manifest Types

#### Machine Manifest (`src/manifests/machine_manifest.arxml`)

Defines the underlying machine/platform configuration:

```xml
<MACHINE>
  <SHORT-NAME>ExampleMachine</SHORT-NAME>
  <DEFAULT-APPLICATION-TIMEOUT>
    <ENTER-TIMEOUT-VALUE>5</ENTER-TIMEOUT-VALUE>
    <EXIT-TIMEOUT-VALUE>5</EXIT-TIMEOUT-VALUE>
  </DEFAULT-APPLICATION-TIMEOUT>
  <!-- Function groups, modules, processes... -->
</MACHINE>
```

**Contains**:
- Default startup/shutdown timeouts
- Operating system and resource configuration
- Function-group definitions and state machines
- Machine-level policies (trusted platform, security modes)

#### Execution Manifest (`apps/*/manifests/app_execution_manifest.arxml`)

Defines per-application execution requirements:

```xml
<APPLICATION>
  <SHORT-NAME>TirePressureProvider</SHORT-NAME>
  <SERVICE-MAPPINGS>
    <SERVICE-MAPPING>
      <SERVICE-ID>...</SERVICE-ID>
      <INSTANCE-ID>...</INSTANCE-ID>
    </SERVICE-MAPPING>
  </SERVICE-MAPPINGS>
  <STARTUP-CONFIGS>
    <!-- Dependencies and startup behavior -->
  </STARTUP-CONFIGS>
</APPLICATION>
```

**Contains**:
- Service interfaces exposed by the application
- Instance mappings to binding endpoints
- Startup dependencies and function-group triggers
- Process launch parameters and environment variables

### Manifest Processing Pipeline

The build system processes manifests through a multi-stage pipeline:

#### 1. Validation

Verify ARXML against AUTOSAR schema:

```bash
python3 scripts/manifests/verify_arxml.py \
  src/manifests/machine_manifest.arxml \
  src/manifests/schema/AUTOSAR_00054.xsd
```

Catches schema violations early in the build.

#### 2. Conversion (ARXML → JSON)

Transform XML manifests to JSON for runtime loading:

```bash
python3 scripts/manifests/arxml2json.py \
  src/manifests/machine_manifest.arxml \
  build/manifests/machine_manifest.json

python3 scripts/manifests/arxml2json.py \
  apps/02_tire_pressure/manifests/app_execution_manifest.arxml \
  build/manifests/app_execution_manifest.json
```

**Converter Features**:
- Auto-detects manifest type (machine vs execution)
- Supports multiple AUTOSAR versions (r4.0, r25, r51, r54)
- Preserves AUTOSAR semantic structure in JSON output
- Comprehensive logging for debugging

#### 3. Merging (Optional)

Combine platform and application manifests into unified configuration:

```bash
python3 scripts/manifests/merge_execution_manifests.py \
  build/manifests/machine_manifest.json \
  build/manifests/merged_manifests.json \
  build/manifests/app_execution_manifest.json
```

**Merge Behavior**:
- Validates cross-references between manifests
- Combines function-group definitions with startup configs
- Produces single runtime-consumable manifest

#### 4. Runtime Loading

Execution manager loads processed JSON at startup:

```bash
./build/Release/bin/ara_exec build/manifests/machine_manifest.json
```

Runtime benefits:
- JSON is lightweight and easier to parse than XML
- Schema separation enables flexible tooling
- Manifest processing is **not** on the critical path

### CMake Integration

The build system automates manifest processing:

```cmake
add_custom_target(process_platform_manifests ALL
    COMMAND python3 scripts/manifests/verify_arxml.py ...
    COMMAND python3 scripts/manifests/arxml2json.py ...
)
```

**When you run**:
```bash
./scripts/build/build.sh
```

The build **automatically**:
1. ✅ Validates all ARXML files against schema
2. ✅ Converts ARXML → JSON
3. ✅ Places processed manifests in `build/manifests/`

### Manifest Directory Structure

```
src/manifests/
├── machine_manifest.arxml      # Platform configuration
├── execution_manifest.arxml    # Example execution manifest
└── schema/
    └── AUTOSAR_00054.xsd       # XSD validation schema

apps/02_tire_pressure/manifests/
└── app_execution_manifest.arxml  # Tire-pressure app manifest

build/manifests/                 # Processed outputs (auto-generated)
├── machine_manifest.json
├── app_execution_manifest.json
└── merged_manifests.json (optional)
```

### Example: Machine Manifest JSON Output

```json
{
  "machine": {
    "shortName": "ExampleMachine",
    "defaultTimeout": {
      "enterTimeoutValue": 5,
      "exitTimeoutValue": 5
    },
    "functionGroups": [
      {
        "shortName": "MachineFGSet",
        "states": ["Started", "Shutdown"]
      }
    ],
    "modules": [
      {
        "shortName": "LinuxOS",
        "resourceGroups": ["SystemResourceGroup"]
      }
    ]
  }
}
```

### Example: Execution Manifest JSON Output

```json
{
  "executionManifest": {
    "process": {
      "shortName": "TirePressureProvider",
      "executable": "openaa_app_02_tp_provider",
      "reportingBehavior": "AUTOSTART",
      "serviceMappings": [
        {
          "serviceId": "tire_pressure_service",
          "instanceId": "vehicle_0"
        }
      ]
    }
  }
}
```

## Tests And Lint

Build all test targets first:

```bash
./scripts/build/build.sh --build-tests
```

### Unit Tests

The project includes comprehensive unit test coverage for core modules:

```bash
./scripts/test/run_unit_test.sh
```

Test coverage includes:
- **Core Module Tests**: lifecycle, error domains, initialization, abort handlers
- **Communication Tests**: error domains, runtime state, service types
- **Total**: 31+ unit tests covering critical runtime paths

### Smoke Tests

Basic integration and smoke tests:

```bash
./scripts/test/run_smoke_test.sh
```

### Code Quality

Run the formatting check:

```bash
./scripts/lint/run_clang_format_check.sh
```

Or auto-fix formatting issues:

```bash
./scripts/lint/run_clang_format_fix.sh
```

Run the `clang-tidy` static analysis:

```bash
./scripts/lint/run_clang_tidy_check.sh
```

## CI

Pull requests trigger three independent GitHub Actions jobs:

- `clang-checks`: runs `clang-format` and `clang-tidy`
- `unit-tests`: runs `ctest -L unit`
- `smoke-tests`: runs `ctest -L smoke`

Each job writes its own GitHub Job Summary and uploads its raw report or log as an artifact.

## Project Structure

- **`include/ara/`**: Public headers for all AUTOSAR Adaptive APIs (`core`, `exec`, `com`, `log`)
- **`src/ara/`**: Runtime implementation of awareness and execution management
- **`apps/`**: Example Adaptive AUTOSAR applications (02_tire_pressure: consumer/provider demo)
- **`tests/`**: Unit and smoke tests with comprehensive coverage
- **`scripts/`**: Build, CI, test, and manifest-processing utilities

## Suggested Next Steps

- **Out-of-Process Execution**: Split application launch into separate processes with IPC
- **Transport Bindings**: Introduce SOME/IP binding for network-based service discovery
- **State Management**: Enhance function-group state machines and transitions
- **Health & Diagnostics**: Add runtime health monitoring and diagnostic support
- **Manifest Loading**: Full manifest-driven application startup and configuration
