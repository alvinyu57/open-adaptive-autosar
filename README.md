# open-adaptive-autosar
Adaptive AUTOSAR open-source platform for research, prototyping, and lightweight automotive middleware development.

## Overview

This project aims to provide a lightweight and modular implementation of key concepts from **Adaptive AUTOSAR**, including:

- Service-Oriented Communication
- Execution Management
- State Management
- SOME/IP-based communication
- Application lifecycle handling

> [!NOTE]
> This is **NOT a production-ready AUTOSAR stack**, but a simplified open platform for:
> * Learning AUTOSAR Adaptive architecture
> * Rapid prototyping
> * Academic research
> * Middleware experimentation

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
- `--shared`: build shared libraries
- `--build-type <type>`: switch Conan/CMake build type, for example `Debug` or `Release`
- `--conan-option <value>`: pass an extra Conan option and repeat as needed

Build artifacts are generated under `build/<BuildType>`, for example `build/Release`.

### Option 2: Conan Only

```bash
conan profile detect --force
conan install . --output-folder=build --build=missing \
  -s build_type=Release \
  -o '&:shared=False' \
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
  bash -lc "conan profile detect --force && conan install . --output-folder=build --build=missing -s build_type=Release -o '&:build_tests=True' && conan build . --output-folder=build -o '&:build_tests=True'"
```

This keeps the host machine clean and makes Docker the single source of truth for the build environment.

The mapping is:

- Conan `build_examples` -> CMake `OPEN_AA_BUILD_EXAMPLES`
- Conan `build_tests` -> CMake `OPEN_AA_BUILD_TESTS`

## Run

Run the built-in execution manager demo:

```bash
./build/Release/src/ara/exec/ara_exec_demo
```

Or point the execution manager at a different manifest:

```bash
./build/Release/src/ara/exec/ara_exec_demo /path/to/application_manifest.json
```

Run the hello world adaptive application through the execution manager:

```bash
./build/Release/examples/01_hello_world/openaa_example_hello_world_runner
```

## Application Manifest

The execution manager now supports loading a simplified Adaptive AUTOSAR-style application
manifest from JSON. The current in-process runtime recognizes these fields:

- `applicationId`: stable identifier used to resolve the registered application factory
- `shortName`: AUTOSAR-style application short name and expected runtime application name
- `executable`: executable name recorded in the manifest for process-launch alignment
- `arguments`: launch arguments reserved for a future out-of-process runner
- `environmentVariables`: environment key/value pairs reserved for a future process launcher
- `providedServices`: service declarations exposed as manifest metadata
- `autoStart`: whether the execution manager should start the application automatically

Example:

```json
{
  "applicationId": "examples.hello_world",
  "shortName": "examples.hello_world",
  "executable": "openaa_example_hello_world_runner",
  "arguments": [],
  "environmentVariables": {
    "OPENAA_EXAMPLE_PROFILE": "hello_world"
  },
  "providedServices": [
    {
      "serviceId": "examples.hello.greeter",
      "endpoint": "local://hello-greeter"
    }
  ],
  "autoStart": true
}
```

## Tests And Lint

Build all test targets first:

```bash
./scripts/build/build.sh --build-tests
```

Run the unit test binary:

```bash
./scripts/test/run_unit_test.sh
```

Run the smoke test binary:

```bash
./scripts/test/run_smoke_test.sh
```

Run the formatting check:

```bash
docker run --rm \
  -v "$(pwd):/workspace" \
  -w /workspace \
  openaa-build \
  bash -lc "./scripts/lint/run_clang_format_check.sh"
```

Run the `clang-tidy` check:

```bash
docker run --rm \
  -v "$(pwd):/workspace" \
  -w /workspace \
  openaa-build \
  bash -lc "./scripts/lint/run_clang_tidy_check.sh"
```

## CI

Pull requests trigger three independent GitHub Actions jobs:

- `clang-checks`: runs `clang-format` and `clang-tidy`
- `unit-tests`: runs `ctest -L unit`
- `smoke-tests`: runs `ctest -L smoke`

Each job writes its own GitHub Job Summary and uploads its raw report or log as an artifact.

## Suggested Next Steps

- Split application launch into separate processes
- Introduce transport abstractions before implementing SOME/IP
- Expand runtime behavior and middleware coverage beyond the current MVP
