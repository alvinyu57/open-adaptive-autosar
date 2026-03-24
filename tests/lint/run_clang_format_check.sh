#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

mapfile -t SOURCE_FILES < <(
    cd "${PROJECT_ROOT}" && \
    rg --files modules tests examples \
        -g '*.c' \
        -g '*.cc' \
        -g '*.cpp' \
        -g '*.h' \
        -g '*.hpp'
)

if [ "${#SOURCE_FILES[@]}" -eq 0 ]; then
    echo "No source files found for clang-format check."
    exit 0
fi

echo "Running clang-format check on ${#SOURCE_FILES[@]} file(s)..."

cd "${PROJECT_ROOT}"
clang-format --dry-run --Werror --style=file "${SOURCE_FILES[@]}"

echo "clang-format check passed."
