#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
CONAN_BUILD_TYPE="Release"
BUILD_DIR="${PROJECT_ROOT}/build/Release"
BUILD_TESTS="True"
BUILD_APPS="True"
OUTPUT_FILE_PATH="${BUILD_DIR}/results"

usage() {
    cat <<'EOF'
Usage: ./run_clang_tidy.sh [options]

Options:
    --help                        Show this help message
    --output <file>                 Specify output file for clang-tidy results (default: build/Release/results/clang-tidy-result.txt)
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

    conan profile detect --force && 
    conan install "${PROJECT_ROOT}" \
        --build=missing \
        -s build_type=${CONAN_BUILD_TYPE} \
        -o build_apps=${BUILD_APPS} \
        -o build_tests=${BUILD_TESTS}

    conan build . --output-folder=${BUILD_DIR} \
        -o build_apps=${BUILD_APPS} \
        -o build_tests=${BUILD_TESTS}

else
    echo "Clang-tidy build directory already configured."
fi

echo "Fixing clang-tidy issues on ${#SOURCE_FILES[@]} file(s)..."
cd "${PROJECT_ROOT}"

mkdir -p ${OUTPUT_FILE_PATH}
OUTPUT_FILE="${OUTPUT_FILE_PATH}/clang-tidy-result.txt"


if clang-tidy -p "${BUILD_DIR}" "${SOURCE_FILES[@]}" -fix > "${OUTPUT_FILE}" 2>&1; then
    echo "Fixes applied successfully. Report saved to: ${OUTPUT_FILE}"
    exit 0
else
    echo "Clang-tidy found issues that could not be fixed automatically. Please review the report: ${OUTPUT_FILE}"
    exit 1
fi
