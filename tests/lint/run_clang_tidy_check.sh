#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
BUILD_ROOT="${PROJECT_ROOT}/build-clang-tidy"
BUILD_TYPE="Release"
BUILD_DIR="${BUILD_ROOT}/${BUILD_TYPE}"
RESULTS_DIR="${BUILD_ROOT}/results"
RESULT_FILE="${RESULTS_DIR}/clang-tidy-result.txt"
COMPILE_COMMANDS="${BUILD_DIR}/compile_commands.json"

rm -rf "${BUILD_ROOT}" &> /dev/null

cd "${PROJECT_ROOT}"

conan profile detect --force
conan install . --output-folder="${BUILD_ROOT}" --build=missing \
    -s "build_type=${BUILD_TYPE}" \
    -o 'shared=False' \
    -o 'build_examples=False' \
    -o 'build_tests=True'
conan build . --output-folder="${BUILD_ROOT}" \
    -o 'shared=False' \
    -o 'build_examples=False' \
    -o 'build_tests=True'

mkdir -p "${RESULTS_DIR}"
touch "${RESULT_FILE}"

if [ ! -f "${COMPILE_COMMANDS}" ]; then
    echo "compile_commands.json not found: ${COMPILE_COMMANDS}" >&2
    exit 1
fi

mapfile -t SOURCE_FILES < <(
    python3 - "${COMPILE_COMMANDS}" "${PROJECT_ROOT}" <<'PY'
import json
import sys
from pathlib import Path

compile_commands_path = Path(sys.argv[1]).resolve()
project_root = Path(sys.argv[2]).resolve()

with compile_commands_path.open("r", encoding="utf-8") as fh:
    entries = json.load(fh)

allowed_roots = (
    project_root / "modules",
    project_root / "tests",
)

seen = set()
files = []

for entry in entries:
    source_path = Path(entry["file"]).resolve()
    if source_path.suffix not in {".c", ".cc", ".cpp"}:
        continue
    if not any(source_path.is_relative_to(root) for root in allowed_roots):
        continue
    source_str = str(source_path)
    if source_str in seen:
        continue
    seen.add(source_str)
    files.append(source_str)

for path in sorted(files):
    print(path)
PY
)

if [ "${#SOURCE_FILES[@]}" -eq 0 ]; then
    echo "No compiled source files found for clang-tidy check."
    exit 0
fi

echo "Running clang-tidy check on ${#SOURCE_FILES[@]} file(s)..."

if clang-tidy \
    -p="${BUILD_DIR}" \
    --quiet \
    --warnings-as-errors='*' \
    "${SOURCE_FILES[@]}" > "${RESULT_FILE}" 2>&1; then
    echo ""
    echo "No violations found!"
    exit 0
fi

echo ""
echo "clang-tidy violations found!"
echo "Check details in ${RESULT_FILE}"
exit 1
