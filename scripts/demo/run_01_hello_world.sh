#!/bin/bash

set -euo pipefail

if [[ "${1:-}" == "--docker" && "${IN_DOCKER:-}" != "True" ]]; then
    # Ensure it is built
    ./scripts/build/build.sh --build-apps --docker

    export USER_ID=$(id -u)
    export GROUP_ID=$(id -g)
    docker compose build runtime
    docker compose run --rm -e IN_DOCKER=True runtime "$0" "${@:2}"
    exit $?
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
BUILD_ROOT="${PROJECT_ROOT}/build/Release"

"${BUILD_ROOT}/bin/ara_exec"
