#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
BUILD_ROOT="${PROJECT_ROOT}/build/build/Release"

"${BUILD_ROOT}/modules/exec/openaa_exec"
"${BUILD_ROOT}/examples/01_hello_world/openaa_example_hello_world_runner"
