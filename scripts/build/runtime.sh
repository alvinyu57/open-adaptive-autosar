#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

usage() {
    cat <<'EOF'
Usage: ./scripts/build/runtime.sh [options]

Options:
    --help                         Show this help message

EOF
}


while [[ $# -gt 0 ]]; do
    case "$1" in
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

CURRENT_HASH=$(sha256sum "${PROJECT_ROOT}/docker/runtime.Dockerfile" | awk '{print $1}')
LAST_HASH=$(cat .dockerfile.hash 2>/dev/null || echo "")

if [[ "$CURRENT_HASH" != "$LAST_HASH" ]]; then
    echo "Dockerfile changed, rebuilding..."
    
    docker build \
        --build-arg USER_ID="$(id -u)" \
        --build-arg GROUP_ID="$(id -g)" \
        -f "${PROJECT_ROOT}/docker/runtime.Dockerfile" \
        -t openaa-runtime:1.0.0 .

    echo "$CURRENT_HASH" > .dockerfile.hash
else
    echo "Dockerfile no change, skip build"
fi