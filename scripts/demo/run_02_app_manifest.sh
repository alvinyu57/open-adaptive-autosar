#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
BUILD_ROOT="${PROJECT_ROOT}/build/Release"

"${BUILD_ROOT}/src/ara/exec/ara_exec_demo"
"${BUILD_ROOT}/apps/02_app_manifest/openaa_app_manifest_demo"
