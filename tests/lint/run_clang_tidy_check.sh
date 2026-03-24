#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build/Release"
COMPILE_COMMANDS="${BUILD_DIR}/compile_commands.json"
BUILD_TYPE="$(basename "${BUILD_DIR}")"

restore_conan_dependencies() {
    echo "Restoring Conan dependencies for clang-tidy..."
    cd "${PROJECT_ROOT}"
    conan profile detect --force
    conan install . --output-folder=build --build=missing \
        -s "build_type=${BUILD_TYPE}" \
        -o 'shared=False' \
        -o 'build_examples=False' \
        -o 'build_tests=True'
}

compile_commands_reference_missing_paths() {
    python3 - "${COMPILE_COMMANDS}" <<'PY'
import json
import os
import shlex
import sys
from pathlib import Path

compile_commands_path = Path(sys.argv[1]).resolve()

with compile_commands_path.open("r", encoding="utf-8") as fh:
    entries = json.load(fh)

for entry in entries:
    command = entry.get("command")
    arguments = entry.get("arguments")
    tokens = shlex.split(command) if command else list(arguments or [])

    i = 0
    while i < len(tokens):
        token = tokens[i]
        include_path = None

        if token in {"-I", "-isystem"} and i + 1 < len(tokens):
            include_path = tokens[i + 1]
            i += 1
        elif token.startswith("-I") and len(token) > 2:
            include_path = token[2:]
        elif token.startswith("-isystem") and len(token) > len("-isystem"):
            include_path = token[len("-isystem"):]

        if include_path and os.path.isabs(include_path) and not Path(include_path).exists():
            print(include_path)
            raise SystemExit(0)

        i += 1

raise SystemExit(1)
PY
}

if [ ! -f "${COMPILE_COMMANDS}" ]; then
    echo "compile_commands.json not found, building test configuration..." >&2
    "${PROJECT_ROOT}/build.sh" --build-tests
fi

if [ ! -f "${COMPILE_COMMANDS}" ]; then
    echo "compile_commands.json still not found: ${COMPILE_COMMANDS}" >&2
    exit 1
fi

if missing_path="$(compile_commands_reference_missing_paths)"; then
    echo "Missing dependency path referenced by compile_commands.json: ${missing_path}" >&2
    restore_conan_dependencies
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

cd "${PROJECT_ROOT}"
clang-tidy \
    -p "${BUILD_DIR}" \
    --quiet \
    --warnings-as-errors='*' \
    "${SOURCE_FILES[@]}"

echo "clang-tidy check passed."
