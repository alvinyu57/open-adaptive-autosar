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
        openaa-runtime:1.0.0 \
        bash -lc "
            ./scripts/test/$command_name
        "

    exit $?
fi

BUILD_ROOT="${PROJECT_ROOT}/build/Release"

# Run the tire pressure apps in the background
${BUILD_ROOT}/bin/ara_exec &