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

## MVP Scope

This repository now contains a minimal runnable MVP aligned with the current folder layout:

- `ara`: standardized ARA-facing interfaces
- `platform`: OpenAA concrete runtime implementations built behind the ARA interfaces
- `examples/01_hello_world`: a sample adaptive application that registers and exposes a greeting service
- `tests/unit`: GoogleTest-based unit tests for lifecycle and service registration behavior
- `tests/smoke`: smoke coverage for bringing up the execution manager and a test application
- `tests/lint`: `clang-format` and `clang-tidy` helper scripts used locally and in CI

The MVP intentionally keeps everything in-process so the architecture stays easy to understand before introducing IPC, SOME/IP, manifests, or process supervision.

## Build

### Option 1: Helper Script

```bash
./build.sh
```

Enable examples and tests:

```bash
./build.sh --build-examples --build-tests
```

Build a different configuration:

```bash
./build.sh --build-type Debug --shared
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

- Conan `shared` -> CMake `BUILD_SHARED_LIBS`
- Conan `build_examples` -> CMake `OPEN_AA_BUILD_EXAMPLES`
- Conan `build_tests` -> CMake `OPEN_AA_BUILD_TESTS`

## Run

Run the built-in execution manager demo:

```bash
./build/Release/platform/exec/openaa_exec
```

Run the hello world adaptive application through the execution manager:

```bash
./build/Release/examples/01_hello_world/openaa_example_hello_world_runner
```

## Tests And Lint

Build all test targets first:

```bash
./build.sh --build-tests
```

Run the unit test binary:

```bash
./tests/unit/run_unit_test.sh
```

Run the smoke test binary:

```bash
./tests/smoke/run_core_smoke_test.sh
```

Run the formatting check:

```bash
docker run --rm \
  -v "$(pwd):/workspace" \
  -w /workspace \
  openaa-build \
  bash -lc "./tests/lint/run_clang_format_check.sh"
```

Run the `clang-tidy` check:

```bash
docker run --rm \
  -v "$(pwd):/workspace" \
  -w /workspace \
  openaa-build \
  bash -lc "./tests/lint/run_clang_tidy_check.sh"
```

## CI

Pull requests trigger three independent GitHub Actions jobs:

- `clang-checks`: runs `clang-format` and `clang-tidy`
- `unit-tests`: runs `ctest -L unit`
- `smoke-tests`: runs `ctest -L smoke`

Each job writes its own GitHub Job Summary and uploads its raw report or log as an artifact.

## Suggested Next Steps

- Add application manifests and load them from JSON or YAML
- Split application launch into separate processes
- Introduce transport abstractions before implementing SOME/IP
- Expand runtime behavior and middleware coverage beyond the current MVP
