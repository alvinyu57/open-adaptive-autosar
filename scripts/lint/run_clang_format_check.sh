#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

BUILD_IN_DOCKER="False"

usage() {
    cat <<'EOF'
Usage: ./run_clang_format_check.sh [options]

Options:
    --docker                       Check clang-format inside a Docker container
    --help                         Show this help message

Examples:
    ./run_clang_format_check.sh
    ./run_clang_format_check.sh --docker
EOF
}

check_clang_format() {

    if command -v rg >/dev/null 2>&1; then
        mapfile -t SOURCE_FILES < <(
            cd "${PROJECT_ROOT}" && \
            rg --files src tests apps \
                -g '*.c' \
                -g '*.cc' \
                -g '*.cpp' \
                -g '*.h' \
                -g '*.hpp'
        )
    else
        mapfile -t SOURCE_FILES < <(
            find \
                "${PROJECT_ROOT}/src" \
                "${PROJECT_ROOT}/tests" \
                "${PROJECT_ROOT}/apps" \
                -type f \
                \( -name '*.c' -o -name '*.cc' -o -name '*.cpp' -o -name '*.h' -o -name '*.hpp' \) \
                | sort
        )
    fi

    if [ "${#SOURCE_FILES[@]}" -eq 0 ]; then
        echo "No source files found for clang-format check."
        exit 0
    fi

    echo "Running clang-format check on ${#SOURCE_FILES[@]} file(s)..."

    cd "${PROJECT_ROOT}"
    clang-format --dry-run --Werror --style=file "${SOURCE_FILES[@]}"

    echo "clang-format check passed."
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --docker)
            BUILD_IN_DOCKER="True"
            shift
            ;;
        --help)
            usage
            exit 0
            ;;
        *)
            echo "Unknown option: $1" >&2
            usage >&2
            exit 1
            ;;
    esac
done

if [[ ${BUILD_IN_DOCKER} == "True" ]]; then
    echo "Running clang-format check in docker container..."

    # check if docker image already exists to avoid unnecessary rebuilds
    if [[ -n "$(docker images -q openaa-build:latest 2> /dev/null)" ]]; then
        echo "Docker image 'openaa-build:latest' already exists, skipping build."
    else
        echo "Building Docker image 'openaa-build:latest'..."

        docker build \
            --build-arg USER_ID="$(id -u)" \
            --build-arg GROUP_ID="$(id -g)" \
            -f "${PROJECT_ROOT}/Dockerfile" \
            -t openaa-build .
    fi

    docker run --rm \
        -v "${PROJECT_ROOT}:/workspace" \
        -w /workspace \
        openaa-build \
        bash -lc "scripts/lint/run_clang_format_check.sh"
else
    check_clang_format
fi


