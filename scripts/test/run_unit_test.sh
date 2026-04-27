#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

if [[ "${1:-}" == "--docker" && "${IN_DOCKER:-}" != "True" ]]; then
    
    command_name=$(basename "$0")
    
    ${PROJECT_ROOT}/scripts/build/runtime.sh

    docker run --rm \
        -v "${PROJECT_ROOT}:/workspace" \
        -w /workspace \
        openaa-build \
        bash -lc "
            ./scripts/test/$command_name
        "

    exit $?
fi

BUILD_ROOT="${PROJECT_ROOT}/build/Release"
TEST_BINARY="${BUILD_ROOT}/tests/openaa_core_unit_test"

needs_rebuild=false

if [ ! -x "${TEST_BINARY}" ]; then
    needs_rebuild=true
elif find "${PROJECT_ROOT}/tests" "${PROJECT_ROOT}/include" "${PROJECT_ROOT}/src/ara" \
    -type f -newer "${TEST_BINARY}" | grep -q .; then
    needs_rebuild=true
fi

if [ "${needs_rebuild}" = true ] && [ "${IN_DOCKER:-}" != "True" ]; then
    echo "unit test binary missing or stale, building test target..." >&2
    "${PROJECT_ROOT}/scripts/build/build.sh" --build-tests
fi

if [ ! -x "${TEST_BINARY}" ]; then
    echo "unit test binary still not found after build: ${TEST_BINARY}" >&2
    exit 1
fi

"${TEST_BINARY}"
