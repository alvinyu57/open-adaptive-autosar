#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
CONAN_BUILD_TYPE="Release"
BUILD_DIR="${PROJECT_ROOT}/build/${CONAN_BUILD_TYPE}"
OUTPUT_FILE_PATH="${BUILD_DIR}/results"

BUILD_IN_DOCKER="False"

usage() {
    cat <<'EOF'
Usage: ./run_clang_tidy.sh [options]

Options:
    --docker                      Run clang-tidy in a Docker container
    --help                        Show this help message
    --output <file>               Specify output file for clang-tidy results (default: build/Release/results/clang-tidy-result.txt)
EOF
}

check_calng_tidy() {

    if [ ! -f "${BUILD_DIR}/compile_commands.json" ]; then
        echo "Generating compile_commands.json..."
        "${PROJECT_ROOT}/scripts/build/build.sh" --build-tests
    fi

    mkdir -p "${OUTPUT_FILE_PATH}"
    OUTPUT_FILE="${OUTPUT_FILE_PATH}/clang-tidy-result.txt"

    if run-clang-tidy -p "${BUILD_DIR}" \
        -header-filter=".*" \
        -j "$(nproc)" \
        -quiet \
        "src/.*|apps/.*|tests/.*" > "${OUTPUT_FILE}" 2>&1; then

        echo "No violations found!"
    else
        echo "Violations found! Check: ${OUTPUT_FILE}"
        cat "${OUTPUT_FILE}"

        exit 1
    fi

}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --output)
            OUTPUT_FILE_PATH="$2"
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
    echo "Running clang-tidy in Docker container..."
    docker run \
        --rm \
        -v "${PROJECT_ROOT}:/workspace" \
        -w /workspace openaa-build \
        bash -lc "\
            ./scripts/lint/run_clang_tidy_fix.sh
        "
else
    echo "Running clang-tidy locally..."
    check_calng_tidy
fi


