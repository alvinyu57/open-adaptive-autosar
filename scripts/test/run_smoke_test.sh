#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

if [[ "${1:-}" == "--docker" ]]; then
    
    command_name=$(basename "$0")

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
TEST_BINARY="${BUILD_ROOT}/tests/openaa_core_smoke_test"

needs_rebuild=false

if [ ! -x "${TEST_BINARY}" ]; then
    needs_rebuild=true
elif find "${PROJECT_ROOT}/tests" "${PROJECT_ROOT}/include" "${PROJECT_ROOT}/src/ara" \
    -type f -newer "${TEST_BINARY}" | grep -q .; then
    needs_rebuild=true
fi

if [ "${needs_rebuild}" = true ]; then
    echo "core smoke test binary missing or stale, building test target..." >&2
    "${PROJECT_ROOT}/scripts/build/build.sh" --build-tests
fi

if [ ! -x "${TEST_BINARY}" ]; then
    echo "core smoke test binary still not found after build: ${TEST_BINARY}" >&1
    exit 1
fi

echo "--------------------------------------------------------"
echo "Running Core Smoke Test..."
echo "Binary: ${TEST_BINARY}"
echo "--------------------------------------------------------"

if "${TEST_BINARY}"; then
    echo "--------------------------------------------------------"
    echo "SUCCESS: Core Smoke Test completed successfully."
    echo "--------------------------------------------------------"
else
    EXIT_CODE=$?
    echo "--------------------------------------------------------"
    echo "FAILURE: Core Smoke Test failed with exit code ${EXIT_CODE}."
    echo "--------------------------------------------------------"
    exit ${EXIT_CODE}
fi
