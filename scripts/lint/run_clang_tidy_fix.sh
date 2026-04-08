#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build/Release"

usage() {
    cat <<'EOF'
Usage: ./run_clang_tidy_fix.sh [options]

Options:
    --help                        Show this help message
EOF
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --output)
            OUTPUT_FILE_PATH="$2"
            shift 2
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

if [ ! -f "${BUILD_DIR}/compile_commands.json" ]; then
    echo "Building project to generate compile_commands.json..."
    "${PROJECT_ROOT}/scripts/build/build.sh" --build-tests
fi

echo "Starting clang-tidy fix..."
cd "${PROJECT_ROOT}"

run-clang-tidy -p "${BUILD_DIR}" \
    -fix \
    -format \
    -header-filter=".*" \
    -j "$(nproc)" \
    "src/.*|apps/.*|tests/.*"
