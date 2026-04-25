#pragma once

#include <cstddef>

struct BenchmarkResult {
    double throughputOpsPerSec = 0.0;

    double avgLatencyNs = 0.0;
    double p50Ns = 0.0;
    double p95Ns = 0.0;
    double p99Ns = 0.0;
    double p999Ns = 0.0;
    double maxNs = 0.0;

    std::size_t failedPushes = 0;
    std::size_t failedPops = 0;
};