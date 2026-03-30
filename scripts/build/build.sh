#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

CONAN_BUILD_TYPE="Release"
CONAN_OPTIONS=""

usage() {
    cat <<'EOF'
Usage: ./build.sh [options]

Options:
  --build-apps                   Enable application targets
  --build-tests                  Enable test targets
  --shared                       Build shared libraries
  --build-type <type>            Conan build type, e.g. Release or Debug
  --conan-option <value>         Extra Conan option, can be passed multiple times
  --help                         Show this help message

Examples:
  ./build.sh --build-apps --build-tests
  ./build.sh --build-type Debug --shared
  ./build.sh --conan-option "-o &:some_other_option=True"
EOF
}

BUILD_APPS="False"
BUILD_TESTS="False"
BUILD_SHARED="False"

EXTRA_CONAN_OPTIONS=()

while [[ $# -gt 0 ]]; do
    case "$1" in
        --build-apps)
            BUILD_APPS="True"
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
    bash -lc "
        echo 'Build configuration:' &&
        echo '  build_type=${CONAN_BUILD_TYPE}' &&
        echo '  shared=${BUILD_SHARED}' &&
        echo '  build_apps=${BUILD_APPS}' &&
        echo '  build_tests=${BUILD_TESTS}' &&

        rm -rf build/${CONAN_BUILD_TYPE} &&

        conan profile detect --force && 

        conan install . --output-folder=build --build=missing \
            -s build_type=${CONAN_BUILD_TYPE} \
            -o 'shared=${BUILD_SHARED}' \
            -o 'build_apps=${BUILD_APPS}' \
            -o 'build_tests=${BUILD_TESTS}' \
            ${CONAN_OPTIONS} &&

        echo 'Generated preset values:' &&
        sed -n '/cacheVariables/,/}/p' build/${CONAN_BUILD_TYPE}/generators/CMakePresets.json &&

        conan build . --output-folder=build \
            -o 'shared=${BUILD_SHARED}' \
            -o 'build_apps=${BUILD_APPS}' \
            -o 'build_tests=${BUILD_TESTS}'
    "