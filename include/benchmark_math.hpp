#pragma once

#include <cstddef>
#include <vector>

#include "benchmark_result.hpp"

double percentile_ns(const std::vector<double>& sortedSamples, double percentile) {
    if (sortedSamples.empty())
        return 0.0;

    const auto index = static_cast<std::size_t>((percentile / 100.0) *
                                                static_cast<double>(sortedSamples.size() - 1));

    return sortedSamples[index];
}

double max_ns(const std::vector<double>& sortedSamples) {
    if (sortedSamples.empty())
        return 0.0;
    return sortedSamples.back();
}

BenchmarkResult average_results(const std::vector<BenchmarkResult>& results) {
    BenchmarkResult avg;
    if (results.empty())
        return avg;

    for (const auto& result : results) {
        avg.throughputOpsPerSec += result.throughputOpsPerSec;
        avg.avgLatencyNs += result.avgLatencyNs;
        avg.p50Ns += result.p50Ns;
        avg.p95Ns += result.p95Ns;
        avg.p99Ns += result.p99Ns;
        avg.p999Ns += result.p999Ns;
        avg.maxNs += result.maxNs;
        avg.failedPushes += result.failedPushes;
        avg.failedPops += result.failedPops;
    }

    const double count = static_cast<double>(results.size());

    avg.throughputOpsPerSec /= count;
    avg.avgLatencyNs /= count;
    avg.p50Ns /= count;
    avg.p95Ns /= count;
    avg.p99Ns /= count;
    avg.p999Ns /= count;
    avg.maxNs /= count;

    avg.failedPushes /= results.size();
    avg.failedPops /= results.size();

    return avg;
}