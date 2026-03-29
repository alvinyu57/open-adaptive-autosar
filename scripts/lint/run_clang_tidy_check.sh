#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build/Release"
BUILD_TESTS="ON"
BUILD_APPS="ON"
OUTPUT_FILE="${BUILD_DIR}/results/clang-tidy-result.txt"

usage() {
    cat <<'EOF'
Usage: ./run_clang_tidy.sh [options]

Options:
  --help                        Show this help message
  --output <file>                 Specify output file for clang-tidy results (default: build/Release/results/clang-tidy-result.txt)
EOF
}

if ! command -v clang-tidy >/dev/null 2>&1; then
    echo "clang-tidy is not installed or not available in PATH." >&2
    exit 1
fi

if ! command -v cmake >/dev/null 2>&1; then
    echo "cmake is not installed or not available in PATH." >&2
    exit 1
fi

if ! command -v conan >/dev/null 2>&1; then
    echo "conan is not installed or not available in PATH." >&2
    exit 1
fi

while [[ $# -gt 0 ]]; do
    case "$1" in
        --output)
            OUTPUT_FILE="$2"
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

declare -a SOURCE_ROOTS=()

for candidate in src apps tests; do
    if [ -d "${PROJECT_ROOT}/${candidate}" ]; then
        SOURCE_ROOTS+=("${candidate}")
    fi
done

if [ "${#SOURCE_ROOTS[@]}" -eq 0 ]; then
    echo "No source roots found for clang-tidy."
    exit 0
fi

if command -v rg >/dev/null 2>&1; then
    mapfile -t SOURCE_FILES < <(
        cd "${PROJECT_ROOT}" && \
        rg --files "${SOURCE_ROOTS[@]}" \
            -g '*.c' \
            -g '*.cc' \
            -g '*.cpp' \
            -g '*.cxx' | \
        sed "s|^|${PROJECT_ROOT}/|"
    )
else
    mapfile -t SOURCE_FILES < <(
        find "${SOURCE_ROOTS[@]/#/${PROJECT_ROOT}/}" \
            -type f \
            \( -name '*.c' -o -name '*.cc' -o -name '*.cpp' -o -name '*.cxx' \) \
            -printf '%p\n' \
            | sort
    )
fi

if [ "${#SOURCE_FILES[@]}" -eq 0 ]; then
    echo "No source files found for clang-tidy."
    exit 0
fi

if [ ! -f "${BUILD_DIR}/compile_commands.json" ]; then
    echo "Clang-tidy build directory not found. Configuring and generating compile_commands.json..."
    cmake -S "${PROJECT_ROOT}" -B "${BUILD_DIR}" \
        -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
        -DOPEN_AA_BUILD_APPS=${BUILD_APPS} \
        -DOPEN_AA_BUILD_TESTS="${BUILD_TESTS}"
else
    echo "Clang-tidy build directory already configured."
fi

echo "Running clang-tidy on ${#SOURCE_FILES[@]} file(s)..."
cd "${PROJECT_ROOT}"

if clang-tidy -p "${BUILD_DIR}" "${SOURCE_FILES[@]}" > "${OUTPUT_FILE}" 2>&1; then
    echo "No violations found!"
    echo "Report saved to: ${OUTPUT_FILE}"
    exit 0
else
    echo "clang-tidy violations found!"
    echo "Report saved to: ${OUTPUT_FILE}"
    echo "Full violation report:"
    cat "${OUTPUT_FILE}"
    exit 1
fi
