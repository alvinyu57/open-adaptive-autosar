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
    --docker                       Build inside a Docker container
    --build-apps                   Enable application targets
    --build-tests                  Enable test targets
    --build-type <type>            Conan build type, e.g. Release or Debug
    --conan-option <value>         Extra Conan option, can be passed multiple times
    --help                         Show this help message

Examples:
    ./build.sh --build-apps --build-tests
    ./build.sh --build-apps --build-tests --docker
    ./build.sh --build-type Debug
    ./build.sh --conan-option "-o &:some_other_option=True"
EOF
}

BUILD_IN_DOCKER="False"

BUILD_APPS="False"
BUILD_TESTS="False"

EXTRA_CONAN_OPTIONS=()

while [[ $# -gt 0 ]]; do
    case "$1" in
        --docker)
            BUILD_IN_DOCKER="True"
            shift
            ;;
        --build-apps)
            BUILD_APPS="True"
            shift
            ;;
        --build-tests)
            BUILD_TESTS="True"
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

if [[ ${BUILD_IN_DOCKER} == "True" && ${IN_DOCKER:-False} == "False" ]]; then
    echo "Building in docker container..."

    # Filter out --docker from arguments to prevent recursion
    args=()
    for arg in "$@"; do
        if [[ "$arg" != "--docker" ]]; then
            args+=("$arg")
        fi
    done

    export USER_ID=$(id -u)
    export GROUP_ID=$(id -g)
    
    docker compose build build
    docker compose run --rm -e IN_DOCKER=True build ./scripts/build/build.sh "${args[@]}"
    exit $?
fi


# Building natively...
echo "Building natively..."

    rm -rf build/${CONAN_BUILD_TYPE}

    conan profile detect --force && 

    conan install . --output-folder=build --build=missing \
        -s build_type=${CONAN_BUILD_TYPE} \
        -o *:build_apps=${BUILD_APPS} \
        -o *:build_tests=${BUILD_TESTS} \
        ${CONAN_OPTIONS}
    
    conan build . --output-folder=build \
        -o *:build_apps=${BUILD_APPS} \
        -o *:build_tests=${BUILD_TESTS}
