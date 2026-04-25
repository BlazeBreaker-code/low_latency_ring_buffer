#pragma once

#include <cstddef>

struct ThroughputResult {
    double opsPerSec = 0.0;
    std::size_t failedPushes = 0;
    std::size_t failedPops = 0;
};