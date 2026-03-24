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

- `modules/core`: basic runtime primitives for logging, lifecycle, and service registration
- `modules/exec`: a lightweight execution manager that initializes, starts, and stops applications
- `examples/01_hello_world`: a sample adaptive application that registers and exposes a greeting service

The MVP intentionally keeps everything in-process so the architecture stays easy to understand before introducing IPC, SOME/IP, manifests, or process supervision.

## Build

### Option 1: Local CMake

```bash
cmake -S . -B build
cmake --build build
```

### Option 2: Conan Only

```bash
conan install . --output-folder=build --build=missing \
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
  bash -lc "conan profile detect --force && conan install . --output-folder=build --build=missing && conan build . --output-folder=build"
```

This keeps the host machine clean and makes Docker the single source of truth for the build environment.

If you prefer, you can also use the helper script:

```bash
./build.sh
```

Supported build toggles:

- `BUILD_SHARED=True ./build.sh` builds shared libraries
- `BUILD_EXAMPLES=True ./build.sh` enables example targets
- `BUILD_TESTS=True ./build.sh` enables test targets
- `CONAN_BUILD_TYPE=Debug ./build.sh` switches build type
- `CONAN_OPTIONS="-o '&:some_other_option=True'" ./build.sh` passes extra Conan options

The mapping is:

- Conan `shared` -> CMake `BUILD_SHARED_LIBS`
- Conan `build_examples` -> CMake `OPEN_AA_BUILD_EXAMPLES`
- Conan `build_tests` -> CMake `OPEN_AA_BUILD_TESTS`

## Run

Run the built-in execution manager demo:

```bash
./build/build/Release/modules/exec/openaa_exec
```

Run the hello world adaptive application through the execution manager:

```bash
./build/build/Release/examples/01_hello_world/openaa_example_hello_world_runner
```

## Suggested Next Steps

- Add application manifests and load them from JSON or YAML
- Split application launch into separate processes
- Introduce transport abstractions before implementing SOME/IP
- Add unit tests for lifecycle and service registration
