#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
BUILD_ROOT="${PROJECT_ROOT}/build"
BUILD_TYPE="Release"
BUILD_DIR="${BUILD_ROOT}/${BUILD_TYPE}"
RESULTS_DIR="${BUILD_ROOT}/results"
RESULT_FILE="${RESULTS_DIR}/clang-tidy-result.txt"
COMPILE_COMMANDS="${BUILD_DIR}/compile_commands.json"
NORMALIZED_DB_DIR="${RESULTS_DIR}/clang-tidy-db"
NORMALIZED_COMPILE_COMMANDS="${NORMALIZED_DB_DIR}/compile_commands.json"

cd "${PROJECT_ROOT}"

if [ -d "${BUILD_DIR}" ]; then
    echo "Reuse existing build directory: ${BUILD_DIR}"
else
    conan profile detect --force
    conan install . --output-folder="${BUILD_ROOT}" --build=missing \
        -s "build_type=${BUILD_TYPE}" \
        -o 'shared=False' \
        -o 'build_apps=False' \
        -o 'build_tests=True'

    cmake -S "${PROJECT_ROOT}" -B "${BUILD_DIR}" \
        -DCMAKE_TOOLCHAIN_FILE="${BUILD_DIR}/generators/conan_toolchain.cmake" \
        -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
        -DBUILD_SHARED_LIBS=OFF \
        -DOPEN_AA_BUILD_APPS=OFF \
        -DOPEN_AA_BUILD_TESTS=ON
fi

mkdir -p "${RESULTS_DIR}"
: > "${RESULT_FILE}"

if [ ! -f "${COMPILE_COMMANDS}" ]; then
    echo "compile_commands.json not found: ${COMPILE_COMMANDS}" >&2
    exit 1
fi

mkdir -p "${NORMALIZED_DB_DIR}"

PROJECT_ROOT="${PROJECT_ROOT}" \
BUILD_DIR="${BUILD_DIR}" \
NORMALIZED_COMPILE_COMMANDS="${NORMALIZED_COMPILE_COMMANDS}" \
python3 - "${COMPILE_COMMANDS}" <<'PY'
import json
import os
import shlex
import sys
from pathlib import Path

compile_commands_path = Path(sys.argv[1]).resolve()
project_root = Path(os.environ["PROJECT_ROOT"]).resolve()
build_dir = Path(os.environ["BUILD_DIR"]).resolve()
normalized_output = Path(os.environ["NORMALIZED_COMPILE_COMMANDS"]).resolve()


def remap_path_string(value: str) -> str:
    return (
        value.replace("/workspace/build/Release", str(build_dir))
        .replace("/workspace", str(project_root))
    )


def remap_entry(entry: dict) -> dict:
    normalized = dict(entry)
    normalized["file"] = remap_path_string(entry["file"])
    normalized["directory"] = remap_path_string(entry["directory"])

    if "command" in entry:
        command_parts = shlex.split(entry["command"])
        normalized["command"] = shlex.join(
            [remap_path_string(part) for part in command_parts]
        )

    if "arguments" in entry:
        normalized["arguments"] = [
            remap_path_string(argument) for argument in entry["arguments"]
        ]

    return normalized


with compile_commands_path.open("r", encoding="utf-8") as fh:
    entries = json.load(fh)

normalized_entries = [remap_entry(entry) for entry in entries]
normalized_output.write_text(
    json.dumps(normalized_entries, indent=2) + "\n",
    encoding="utf-8",
)
PY

mapfile -t SOURCE_FILES < <(
    PROJECT_ROOT="${PROJECT_ROOT}" python3 - "${NORMALIZED_COMPILE_COMMANDS}" <<'PY'
import json
import os
import sys
from pathlib import Path

compile_commands_path = Path(sys.argv[1]).resolve()
project_root = Path(os.environ["PROJECT_ROOT"]).resolve()

with compile_commands_path.open("r", encoding="utf-8") as fh:
    entries = json.load(fh)

allowed_roots = (
    project_root / "src",
    project_root / "apps",
    project_root / "tests",
)

seen = set()
files = []

for entry in entries:
    source_path = Path(entry["file"]).resolve()
    if source_path.is_absolute() and source_path.parts[:2] == ("/", "workspace"):
        source_path = project_root.joinpath(*source_path.parts[2:])
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
    -p="${NORMALIZED_DB_DIR}" \
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
