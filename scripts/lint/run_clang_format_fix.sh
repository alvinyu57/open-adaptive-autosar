#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
CONAN_BUILD_TYPE="Release"
BUILD_DIR="${PROJECT_ROOT}/build/${CONAN_BUILD_TYPE}"
OUTPUT_FILE_PATH="${BUILD_DIR}/lint-results"
OUTPUT_FILE="${OUTPUT_FILE_PATH}/clang-format-result.txt"

BUILD_IN_DOCKER="False"

usage() {
    cat <<'EOF'
Usage: ./run_clang_format_fix.sh [options]

Options:
    --docker                       Fix clang-format inside a Docker container
    --help                         Show this help message

Examples:
    ./scripts/lint/run_clang_format_fix.sh
    ./scripts/lint/run_clang_format_fix.sh --docker
    ./scripts/lint/run_clang_format_fix.sh --output build/Release/clang-results/clang-format-result.txt
EOF
}


while [[ $# -gt 0 ]]; do
    case "$1" in
        --output)
            OUTPUT_FILE="$2"
            OUTPUT_FILE_PATH="$(dirname "$2")"
            shift 2
            ;;
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

    command_name=$(basename "$0")

    docker run --rm \
        -v "${PROJECT_ROOT}:/workspace" \
        -w /workspace \
        openaa-build \
        bash -lc "scripts/lint/$command_name"

    exit $?
fi

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

echo "Fixing clang-format check on ${#SOURCE_FILES[@]} file(s)..."

cd "${PROJECT_ROOT}"
clang-format -i --style=file "${SOURCE_FILES[@]}"

echo "clang-format fix applied."

