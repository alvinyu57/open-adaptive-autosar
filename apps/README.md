# Applications

Example Adaptive AUTOSAR applications demonstrating core runtime concepts and service-oriented communication.

Only enable one app at one time in the CMakeLists.txt:

```cmake
# add_subdirectory(01_hello_world)
add_subdirectory(01_hello_world)
```

## 01 - Hello World (Archived)

A minimal "Hello World" Adaptive AUTOSAR application.

**Status**: Archived (disabled in CMakeLists.txt pending documentation updates)

**Purpose**: Demonstrates basic application lifecycle and initialization

**Files**:
- `main.cpp`: Minimal application entry point
- `manifests/app_execution_manifest.arxml`: AUTOSAR execution manifest example

## 02 - Tire Pressure Service (Active)

A **consumer/provider** example demonstrating service-oriented communication in Adaptive AUTOSAR.

### Architecture

The example models a tire-pressure monitoring system:

- **Provider**: Reads tire-pressure data from JSON file and exposes via skeleton API
- **Consumer**: Requests tire-pressure samples from provider proxy

### Components

- **`main_tp_provider.cpp`**: Service provider implementation
  - Loads sample data from `data/tire_pressure_data.json`
  - Implements tire-pressure service skeleton
  - Reports execution state changes
  - Runs indefinitely, serving consumer requests

- **`main_tp_consumer.cpp`**: Service consumer implementation
  - Discovers and connects to tire-pressure service
  - Requests and displays tire-pressure samples
  - Demonstrates synchronous request/response pattern
  - Exits after processing samples

### Common Headers

- **`tp_provider_common.h`**: Shared types and constants (TirePressureSample, tire positions)
- **`tp_provider_skeleton.h`**: Service server-side API (skeleton) for implementation
- **`tp_consumer_proxy.h`**: Service client-side API (proxy) for consumption

### Data

- **`data/tire_pressure_data.json`**: Sample tire-pressure readings for realistic simulation

### Manifests

- **`manifests/app_execution_manifest.arxml`**: AUTOSAR-compliant execution manifest

## Building Applications

Build with applications enabled:

```bash
./scripts/build/build.sh --build-apps
```

Binaries are output to `build/Release/apps/`.

## Running the Tire Pressure Example

```bash
./scripts/demo/run_02_tire_presure.sh
```