#pragma once

#include "tp_provider_common.h"

namespace openaa::tire_pressure {

inline bool HasLowTirePressure(const TirePressureSample& sample, int threshold_kpa) {
    for (const auto& reading : sample.readings) {
        if (reading.pressure_kpa < threshold_kpa) {
            return true;
        }
    }

    return false;
}

} // namespace openaa::tire_pressure
