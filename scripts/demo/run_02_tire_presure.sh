#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

if [[ "${1:-}" == "--docker" ]]; then
    
    command_name=$(basename "$0")

    ${PROJECT_ROOT}/scripts/build/runtime.sh

    docker run --rm \
        -v "${PROJECT_ROOT}:/workspace" \
        -w /workspace \
        openaa-runtime:1.0.0 \
        bash -lc "
            ./scripts/demo/$command_name
        "
        
    exit $?
fi

BUILD_ROOT="${PROJECT_ROOT}/build/Release"

# Run the tire pressure apps in the background
${BUILD_ROOT}/bin/ara_exec &

# Change the value in data/tire_pressure_data.json to simulate different tire pressure readings
sleep 3
sed -i 's/"pressureKPa": 120/"pressureKPa": 99/g' "${BUILD_ROOT}/apps/02_tire_pressure/data/tire_pressure_data.json"

# Wait for 3 seconds and change another value
sleep 3
sed -i 's/"pressureKPa": 107/"pressureKPa": 80/g' "${BUILD_ROOT}/apps/02_tire_pressure/data/tire_pressure_data.json"

# Wait for 3 seconds and change another value
sleep 3
sed -i 's/"pressureKPa": 104/"pressureKPa": 75/g' "${BUILD_ROOT}/apps/02_tire_pressure/data/tire_pressure_data.json"

# Wait for 3 seconds and change another value
sleep 3
sed -i 's/"pressureKPa": 101/"pressureKPa": 70/g' "${BUILD_ROOT}/apps/02_tire_pressure/data/tire_pressure_data.json"

# Wait for 3 seconds and change them back
sleep 3
sed -i 's/"pressureKPa": 99/"pressureKPa": 120/g' "${BUILD_ROOT}/apps/02_tire_pressure/data/tire_pressure_data.json"
sed -i 's/"pressureKPa": 80/"pressureKPa": 107/g' "${BUILD_ROOT}/apps/02_tire_pressure/data/tire_pressure_data.json"
sed -i 's/"pressureKPa": 75/"pressureKPa": 104/g' "${BUILD_ROOT}/apps/02_tire_pressure/data/tire_pressure_data.json"
sed -i 's/"pressureKPa": 70/"pressureKPa": 101/g' "${BUILD_ROOT}/apps/02_tire_pressure/data/tire_pressure_data.json"

# Stop the app
sleep 1
killall ara_exec

sleep 2
exit 0