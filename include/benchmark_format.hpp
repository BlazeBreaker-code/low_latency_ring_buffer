#pragma once

#include <iomanip>
#include <iostream>
#include <string>

#include "benchmark_result.hpp"
#include "throughput_result.hpp"

inline double to_millions(double value) {
    return value / 1'000'000.0;
}

inline void print_latency_header() {
    std::cout << std::left << std::setw(24) << "Benchmark" << std::right << std::setw(14)
              << "msgs/s(M)" << std::setw(12) << "avg(ns)" << std::setw(10) << "p50"
              << std::setw(10) << "p95" << std::setw(10) << "p99" << std::setw(12) << "p99.9"
              << std::setw(12) << "max" << std::setw(16) << "fail_push" << std::setw(16)
              << "fail_pop" << '\n';

    std::cout << std::string(136, '-') << '\n';
}

inline void print_latency_row(const std::string& name, const BenchmarkResult& r) {
    std::cout << std::left << std::setw(24) << name << std::right << std::fixed
              << std::setprecision(2) << std::setw(14) << to_millions(r.throughputOpsPerSec)
              << std::setw(12) << r.avgLatencyNs << std::setw(10) << r.p50Ns << std::setw(10)
              << r.p95Ns << std::setw(10) << r.p99Ns << std::setw(12) << r.p999Ns << std::setw(12)
              << r.maxNs << std::setw(16) << r.failedPushes << std::setw(16) << r.failedPops
              << '\n';
}

inline void print_throughput_header() {
    std::cout << std::left << std::setw(24) << "Benchmark" << std::right << std::setw(14)
              << "msgs/s(M)" << std::setw(16) << "fail_push" << std::setw(16) << "fail_pop" << '\n';

    std::cout << std::string(70, '-') << '\n';
}

inline void print_throughput_row(const std::string& name,
                                 double avgOps,
                                 std::size_t avgFailedPushes,
                                 std::size_t avgFailedPops) {
    std::cout << std::left << std::setw(24) << name << std::right << std::fixed
              << std::setprecision(2) << std::setw(14) << to_millions(avgOps) << std::setw(16)
              << avgFailedPushes << std::setw(16) << avgFailedPops << '\n';
}