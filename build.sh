#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="${SCRIPT_DIR}"

CONAN_BUILD_TYPE="Release"
CONAN_OPTIONS=""

usage() {
    cat <<'EOF'
Usage: ./build.sh [options]

Options:
  --build-examples               Enable example targets
  --build-tests                  Enable test targets
  --shared                       Build shared libraries
  --build-type <type>            Conan build type, e.g. Release or Debug
  --conan-option <value>         Extra Conan option, can be passed multiple times
  --help                         Show this help message

Examples:
  ./build.sh --build-examples --build-tests
  ./build.sh --build-type Debug --shared
  ./build.sh --conan-option "-o &:some_other_option=True"
EOF
}

BUILD_EXAMPLES="False"
BUILD_TESTS="False"
BUILD_SHARED="False"

EXTRA_CONAN_OPTIONS=()

while [[ $# -gt 0 ]]; do
    case "$1" in
        --build-examples)
            BUILD_EXAMPLES="True"
            shift
            ;;
        --build-tests)
            BUILD_TESTS="True"
            shift
            ;;
        --shared)
            BUILD_SHARED="True"
            shift
            ;;
        --build-type)
            CONAN_BUILD_TYPE="$2"
            shift 2
            ;;
        --conan-option)
            EXTRA_CONAN_OPTIONS+=("$2")
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

for extra_option in "${EXTRA_CONAN_OPTIONS[@]}"; do
    if [[ -n "${CONAN_OPTIONS}" ]]; then
        CONAN_OPTIONS="${CONAN_OPTIONS} ${extra_option}"
    else
        CONAN_OPTIONS="${extra_option}"
    fi
done

docker build \
    --build-arg USER_ID="$(id -u)" \
    --build-arg GROUP_ID="$(id -g)" \
    -f "${PROJECT_ROOT}/Dockerfile" \
    -t openaa-build .

docker run --rm \
    -v "${PROJECT_ROOT}:/workspace" \
    -w /workspace \
    openaa-build \
    bash -lc "
        echo 'Build configuration:' &&
        echo '  build_type=${CONAN_BUILD_TYPE}' &&
        echo '  shared=${BUILD_SHARED}' &&
        echo '  build_examples=${BUILD_EXAMPLES}' &&
        echo '  build_tests=${BUILD_TESTS}' &&

        rm -rf build/build/${CONAN_BUILD_TYPE} &&

        conan profile detect --force && 

        conan install . --output-folder=build --build=missing \
            -s build_type=${CONAN_BUILD_TYPE} \
            -o 'shared=${BUILD_SHARED}' \
            -o 'build_examples=${BUILD_EXAMPLES}' \
            -o 'build_tests=${BUILD_TESTS}' \
            ${CONAN_OPTIONS} &&

        echo 'Generated preset values:' &&
        sed -n '/cacheVariables/,/}/p' build/build/${CONAN_BUILD_TYPE}/generators/CMakePresets.json &&

        conan build . --output-folder=build \
            -o 'shared=${BUILD_SHARED}' \
            -o 'build_examples=${BUILD_EXAMPLES}' \
            -o 'build_tests=${BUILD_TESTS}'
    "
