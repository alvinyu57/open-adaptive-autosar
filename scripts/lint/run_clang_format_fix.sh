#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

if command -v rg >/dev/null 2>&1; then
    mapfile -t SOURCE_FILES < <(
        cd "${PROJECT_ROOT}" && \
        rg --files ara platform tests examples \
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

echo "clang-format check passed."
