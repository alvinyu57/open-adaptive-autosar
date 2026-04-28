# Tire Pressure Service Example

A complete consumer/provider service example demonstrating Adaptive AUTOSAR communication patterns.

## Overview

This application models a **tire pressure monitoring system** where:
- The **provider** exposes a service offering tire-pressure readings
- The **consumer** subscribes and periodically requests samples
- Both applications manage execution state and report lifecycle events

This is a realistic example of how Adaptive AUTOSAR applications collaborate through service-oriented APIs.

## Application Structure

```
02_tire_pressure/
├── CMakeLists.txt                 # Build configuration
├── main_tp_provider.cpp           # Service provider process
├── main_tp_consumer.cpp           # Service consumer process
├── tp_provider_common.h          # Shared type definitions
├── tp_consumer_proxy.h           # Consumer-side service API
├── tp_provider_skeleton.h        # Provider-side service API
├── data/
│   └── tire_pressure_data.json   # Sample sensor data
└── manifests/
    └── app_execution_manifest.arxml  # AUTOSAR manifest
```

## Service Definitions

### Tire Pressure Types

**Common Type Definition** (`tp_provider_common.h`):

```cpp
namespace openaa::tire_pressure {
    enum class TirePosition {
        FrontLeft,
        FrontRight,
        RearLeft,
        RearRight
    };
    
    struct TirePressureReading {
        TirePosition position;
        double pressure_kpa;
    };
    
    struct TirePressureSample {
        std::vector<TirePressureReading> readings;
    };
}
```

### Provider Interface

**Skeleton API** (`tp_provider_skeleton.h`):

The provider implements:
- **Service Offering**: Registers tire-pressure service with the runtime
- **Request Handling**: Responds to pressure-sample requests from consumers
- **State Management**: Reports function-group transitions

### Consumer Interface

**Proxy API** (`tp_consumer_proxy.h`):

The consumer:
- **Service Discovery**: Resolves provider instance through ara::com
- **Request/Response**: Synchronously calls pressure-sample operations
- **Error Handling**: Manages timeout and unavailability scenarios

## Data Files

### Tire Pressure Data (`data/tire_pressure_data.json`)

Sample JSON structure:
```json
{
  "samples": [
    {
      "readings": [
        {"position": "FrontLeft", "pressure_kpa": 210.5},
        {"position": "FrontRight", "pressure_kpa": 211.2},
        {"position": "RearLeft", "pressure_kpa": 225.8},
        {"position": "RearRight", "pressure_kpa": 224.9}
      ]
    }
  ]
}
```

The provider:
1. Loads this file at startup
2. Cycles through samples on each consumer request
3. Provides realistic tire-pressure values for testing

## Execution Flow

### Provider (`main_tp_provider.cpp`)

1. **Initialize**: Call `ara::core::Initialize()`
2. **Load Data**: Parse tire-pressure JSON file
3. **Create Execution Client**: Report execution state
4. **Register Service**: Offer tire-pressure service via ara::com
5. **Service Loop**: Wait for and handle consumer requests indefinitely
6. **Cleanup**: Deinitialize on shutdown

### Consumer (`main_tp_consumer.cpp`)

1. **Initialize**: Call `ara::core::Initialize()`
2. **Create Execution Client**: Report execution state
3. **Discover Service**: Find tire-pressure provider via ara::com
4. **Get Proxy**: Create proxy object for provider communication
5. **Request Samples**: Call pressure-sample operations in a loop
6. **Display Results**: Log samples to console
7. **Cleanup**: Deinitialize on completion

## Building

From the repository root:

```bash
# Build with applications enabled
./scripts/build/build.sh --build-apps

# Or build only this application
cd build/Release
cmake --build . --target openaa_app_02_tp_provider openaa_app_02_tp_consumer
```

## Running

### Automated Demo

```bash
./scripts/demo/run_02_tire_presure.sh
```

This script:
1. Starts the provider in the background
2. Waits for initialization
3. Runs the consumer to completion
4. Cleans up the provider process

### Manual Execution

**Terminal 1 - Start Provider**:
```bash
./build/Release/apps/02_tire_pressure/openaa_app_02_tp_provider
```

Expected output:
```
[12:34:56] openaa_app_02_tp_provider: initialized
[12:34:56] ara::exec: Execution client created
[12:34:56] ara::com: Registering tire-pressure service
[12:34:56] Ready to serve requests...
```

**Terminal 2 - Run Consumer**:
```bash
./build/Release/apps/02_tire_pressure/openaa_app_02_tp_consumer
```

Expected output:
```
[12:34:57] openaa_app_02_tp_consumer: initialized
[12:34:57] ara::exec: Execution client created
[12:34:57] Discovering tire-pressure service...
[12:34:57] Found provider, requesting samples...
[12:34:57] Sample 1: FrontLeft=210.5kPa, FrontRight=211.2kPa, RearLeft=225.8kPa, RearRight=224.9kPa
[12:34:58] Consumer completed
```

## Testing Modifications

Common enhancements for learning:

1. **Add Delays**: Insert `std::this_thread::sleep_for()` to simulate network latency
2. **Error Injection**: Return error from provider on specific requests
3. **Dynamic Data**: Read tire-pressure updates from real sensor (would need hardware)
4. **Service Binding**: Add SOME/IP transport layer between provider and consumer
5. **Multiple Consumers**: Run multiple consumer processes simultaneously

## AUTOSAR Manifest

The execution manifest (`manifests/app_execution_manifest.arxml`) declares:
- Process name and executable path
- Service offerings and required bindings
- Startup behavior (auto-start, dependencies)
- Function-group definitions

This manifest is loaded by the execution manager to configure the application lifecycle.

## Design Principles

- **Realistic**: Models actual automotive use cases (vehicle monitoring)
- **Minimal**: Small enough to understand in one sitting
- **Complete**: Demonstrates consumer AND provider patterns
- **Extensible**: Easy to add features (e.g., separate process, SOME/IP binding)
- **Educational**: Excellent reference for building new applications
