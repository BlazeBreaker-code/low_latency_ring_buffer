#include <algorithm>
#include <atomic>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "benchmark_format.hpp"
#include "benchmark_math.hpp"
#include "benchmark_result.hpp"
#include "mutex_queue.hpp"
#include "ring_buffer.hpp"
#include "throughput_result.hpp"
#include "wait_strategy.hpp"

struct TimedMessage {
    using Clock = std::chrono::steady_clock;

    Clock::time_point created_at;
    std::size_t sequence;
};

template <typename QueueType> BenchmarkResult run_single_latency_test(WaitStrategy strategy) {
    using value_type = typename QueueType::value_type;

    constexpr std::size_t message_count = 1'000'000;

    QueueType queue;
    BenchmarkResult result;

    std::atomic<std::size_t> failedPushes{0};
    std::atomic<std::size_t> failedPops{0};

    std::vector<double> latency_samples_ns;
    latency_samples_ns.reserve(message_count);

    auto start = std::chrono::steady_clock::now();

    std::thread producer([&]() {
        for (std::size_t i = 0; i < message_count; ++i) {
            value_type msg{value_type::Clock::now(), i};

            std::size_t spin_count = 0;
            while (!queue.try_push(msg)) {
                failedPushes.fetch_add(1, std::memory_order_relaxed);
                apply_wait_strategy(strategy, spin_count);
            }
        }
    });

    std::thread consumer([&]() {
        std::size_t consumed = 0, spin_count = 0;
        value_type msg{};

        while (consumed < message_count) {
            if (queue.try_pop(msg)) {
                auto now = value_type::Clock::now();

                double latencyNs =
                    std::chrono::duration<double, std::nano>(now - msg.created_at).count();
                latency_samples_ns.push_back(latencyNs);
                ++consumed;
            } else {
                failedPops.fetch_add(1, std::memory_order_relaxed);
                apply_wait_strategy(strategy, spin_count);
            }
        }
    });

    producer.join();
    consumer.join();

    auto end = std::chrono::steady_clock::now();
    double seconds = std::chrono::duration<double>(end - start).count();
    result.throughputOpsPerSec = static_cast<double>(message_count) / seconds;

    std::sort(latency_samples_ns.begin(), latency_samples_ns.end());
    double sum = 0.0;

    for (double sample : latency_samples_ns)
        sum += sample;

    result.avgLatencyNs = sum / static_cast<double>(latency_samples_ns.size());
    result.p50Ns = percentile_ns(latency_samples_ns, 50.0);
    result.p95Ns = percentile_ns(latency_samples_ns, 95.0);
    result.p99Ns = percentile_ns(latency_samples_ns, 99.0);
    result.p999Ns = percentile_ns(latency_samples_ns, 99.9);
    result.maxNs = max_ns(latency_samples_ns);

    result.failedPushes = failedPushes.load(std::memory_order_relaxed);
    result.failedPops = failedPops.load(std::memory_order_relaxed);

    return result;
}

template <typename QueueType>
void run_latency_benchmark(const std::string& name,
                           WaitStrategy strategy,
                           int warmups = 2,
                           int iterations = 5) {
    for (int i = 0; i < warmups; ++i) {
        (void)run_single_latency_test<QueueType>(strategy);
    }

    std::vector<BenchmarkResult> results;
    results.reserve(iterations);

    for (int i = 0; i < iterations; ++i) {
        auto result = run_single_latency_test<QueueType>(strategy);
        results.push_back(result);
    }

    auto avg = average_results(results);
    print_latency_row(name, avg);
}

template <typename QueueType> ThroughputResult run_single_throughput_test(WaitStrategy strategy) {
    using value_type = typename QueueType::value_type;
    constexpr std::size_t message_count = 1'000'000;
    QueueType queue;

    std::atomic<std::size_t> failedPushes{0};
    std::atomic<std::size_t> failedPops{0};

    auto start = std::chrono::steady_clock::now();
    std::thread producer([&]() {
        for (std::size_t i = 0; i < message_count; ++i) {
            std::size_t spin_count = 0;

            while (!queue.try_push(static_cast<value_type>(i))) {
                failedPushes.fetch_add(1, std::memory_order_relaxed);
                apply_wait_strategy(strategy, spin_count);
            }
        }
    });

    std::thread consumer([&]() {
        std::size_t consumed = 0, spin_count = 0;
        value_type value;
        while (consumed < message_count) {
            if (queue.try_pop(value)) {
                ++consumed;
            } else {
                failedPops.fetch_add(1, std::memory_order_relaxed);
                apply_wait_strategy(strategy, spin_count);
            }
        }
    });

    producer.join();
    consumer.join();

    auto end = std::chrono::steady_clock::now();
    double seconds = std::chrono::duration<double>(end - start).count();

    ThroughputResult result;

    result.opsPerSec = static_cast<double>(message_count) / seconds;
    result.failedPushes = failedPushes.load(std::memory_order_relaxed);
    result.failedPops = failedPops.load(std::memory_order_relaxed);

    return result;
}

template <typename QueueType>
void run_throughput_benchmark(const std::string& name, WaitStrategy strategy, int iterations = 5) {
    double totalOps = 0.0;
    std::size_t totalFailedPushes = 0;
    std::size_t totalFailedPops = 0;

    for (int i = 0; i < iterations; ++i) {
        auto result = run_single_throughput_test<QueueType>(strategy);

        totalOps += result.opsPerSec;
        totalFailedPushes += result.failedPushes;
        totalFailedPops += result.failedPops;
    }

    print_throughput_row(
        name, totalOps / iterations, totalFailedPushes / iterations, totalFailedPops / iterations);
}

int main() {
    std::cout << "=== Throughput: Wait Strategy Comparison ===\n";
    print_throughput_header();

    run_throughput_benchmark<RingBuffer<int, 1024>>("Spin", WaitStrategy::Spin);

    run_throughput_benchmark<RingBuffer<int, 1024>>("Yield", WaitStrategy::Yield);

    run_throughput_benchmark<RingBuffer<int, 1024>>("SpinThenYield", WaitStrategy::SpinThenYield);

    std::cout << "\n=== Latency: Wait Strategy Comparison ===\n";
    print_latency_header();

    run_latency_benchmark<RingBuffer<TimedMessage, 1024>>("Spin", WaitStrategy::Spin);

    run_latency_benchmark<RingBuffer<TimedMessage, 1024>>("Yield", WaitStrategy::Yield);

    run_latency_benchmark<RingBuffer<TimedMessage, 1024>>("SpinThenYield",
                                                          WaitStrategy::SpinThenYield);

    return 0;
}