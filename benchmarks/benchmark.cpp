#include <algorithm>
#include <atomic>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "benchmark_result.hpp"
#include "throughput_result.hpp"
#include "mutex_queue.hpp"
#include "ring_buffer.hpp"

struct TimedMessage {
    using clock = std::chrono::steady_clock;

    clock::time_point created_at;
    std::size_t sequence;
};

enum class WaitStrategy {
    Spin,
    Yield, 
    SpinThenYield
};

void apply_wait_strategy(WaitStrategy strategy, std::size_t& spins) {
    switch(strategy) {
        case WaitStrategy::Spin:
            return;

        case WaitStrategy::Yield:
            std::this_thread::yield();
            return;
        
        case WaitStrategy::SpinThenYield: 
            if (++spins >= 100) {
                std::this_thread::yield();
                spins = 0;
            }
            return;
    }
}

double percentile_ns(const std::vector<double>& sortedSamples, double percentile) {
    if (sortedSamples.empty()) return 0.0;

    const auto index = static_cast<std::size_t>(
        (percentile / 100.0) * static_cast<double>(sortedSamples.size() - 1)
    );

    return sortedSamples[index];
}

double max_ns(const std::vector<double>& sortedSamples) {
    if (sortedSamples.empty()) return 0.0;
    return sortedSamples.back();
}

void print_result(const std::string& name, const BenchmarkResult& result) {
    std::cout << name
              << " | throughput: " << result.throughputOpsPerSec << " msgs/sec"
              << " | avg: " << result.avgLatencyNs << " ns"
              << " | p50: " << result.p50Ns << " ns"
              << " | p95: " << result.p95Ns << " ns"
              << " | p99: " << result.p99Ns << " ns"
              << " | p99.9: " << result.p999Ns << " ns"
              << " | max latency: " << result.maxNs << " ns"
              << " | failed pushes: " << result.failedPushes
              << " | failed pops: " << result.failedPops
              << '\n';
}

template<typename QueueType>
BenchmarkResult run_single_latency_test(WaitStrategy strategy) {
    using ValueType = typename QueueType::value_type;

    constexpr std::size_t N = 1'000'000;

    QueueType queue;
    BenchmarkResult result;

    std::atomic<std::size_t> failedPushes{0};
    std::atomic<std::size_t> failedPops{0};

    std::vector<double> samples;
    samples.reserve(N);

    auto start = std::chrono::steady_clock::now();

    std::thread producer([&]() {
        for (std::size_t i = 0; i < N; ++i) {
            ValueType msg {
                ValueType::clock::now(), 
                i
            };
            
            std::size_t spins = 0;
            while (!queue.try_push(msg)) {
                failedPushes.fetch_add(1, std::memory_order_relaxed);
                apply_wait_strategy(strategy, spins);
            }
        }
    });

    std::thread consumer([&]() {
        std::size_t consumed = 0, spins = 0;
        ValueType msg{};
        
        while (consumed < N) {
            if (queue.try_pop(msg)) {
                auto now = ValueType::clock::now();

                double latencyNs = std::chrono::duration<double, std::nano>(now - msg.created_at).count();
                samples.push_back(latencyNs);
                ++consumed;
            } else {
                failedPops.fetch_add(1, std::memory_order_relaxed);
                apply_wait_strategy(strategy, spins);
            }
        }
    });

    producer.join();
    consumer.join();

    auto end = std::chrono::steady_clock::now();
    double seconds = std::chrono::duration<double>(end - start).count();
    result.throughputOpsPerSec = static_cast<double>(N) / seconds;

    std::sort(samples.begin(), samples.end());
    double sum = 0.0;

    for (double sample : samples) sum += sample;
    
    result.avgLatencyNs = sum / static_cast<double>(samples.size());
    result.p50Ns = percentile_ns(samples, 50.0);
    result.p95Ns = percentile_ns(samples, 95.0);
    result.p99Ns = percentile_ns(samples, 99.0);
    result.p999Ns = percentile_ns(samples, 99.9);
    result.maxNs = max_ns(samples);

    result.failedPushes = failedPushes.load(std::memory_order_relaxed);
    result.failedPops = failedPops.load(std::memory_order_relaxed);

    return result;
}

BenchmarkResult average_results(const std::vector<BenchmarkResult>& results) {
    BenchmarkResult avg;
    if (results.empty()) return avg;

    for (const auto& result : results)
    {
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

template<typename QueueType>
void run_latency_benchmark(const std::string& name, WaitStrategy strategy, int warmups = 2, int iterations = 5) {
    for (int i = 0; i < warmups; ++i) {
        (void)run_single_latency_test<QueueType>(strategy);
    }

    std::vector<BenchmarkResult> results;
    results.reserve(iterations);

    for (int i = 0; i < iterations; ++i) {
        auto result = run_single_latency_test<QueueType>(strategy);
        results.push_back(result);

        print_result(name + " run " + std::to_string(i + 1), result);
    }

    auto avg = average_results(results);
    print_result(name + " average ", avg);

    std::cout << '\n';
}

template<typename QueueType>
ThroughputResult run_single_throughput_test(WaitStrategy strategy) {
    using ValueType = typename QueueType::value_type;
    const std::size_t N = 1'000'000;
    QueueType queue;

    std::atomic<std::size_t> failedPushes{0};
    std::atomic<std::size_t> failedPops{0};

    auto start = std::chrono::steady_clock::now();
    std::thread producer([&]() {
        for (std::size_t i = 0; i < N; ++i) {
            std::size_t spins = 0;
            
            while (!queue.try_push(static_cast<ValueType>(i))) {
                // Busy waiting until push is successful
                failedPushes.fetch_add(1, std::memory_order_relaxed);
                apply_wait_strategy(strategy, spins);
            }
        }
    });

    std::thread consumer([&]() {
        std::size_t count = 0, spins = 0;
        ValueType value;
        while (count < N) {
            if (queue.try_pop(value)) {
                ++count;
            } else {
                failedPops.fetch_add(1, std::memory_order_relaxed);
                apply_wait_strategy(strategy, spins);
            }
        }
    });

    producer.join();
    consumer.join();

    auto end = std::chrono::steady_clock::now();
    double seconds = std::chrono::duration<double>(end - start).count();

    ThroughputResult result;

    result.opsPerSec = static_cast<double>(N) / seconds;
    result.failedPushes = failedPushes.load(std::memory_order_relaxed);
    result.failedPops = failedPops.load(std::memory_order_relaxed);

    return result;
}

template<typename QueueType>
void run_throughput_benchmark(const std::string& name, WaitStrategy strategy, int iterations = 5) {
    double totalOps = 0.0;
    std::size_t totalFailedPushes = 0;
    std::size_t totalFailedPops = 0;
    for (int i = 0; i < iterations; ++i) {
        auto result = run_single_throughput_test<QueueType>(strategy);

        totalOps += result.opsPerSec;
        totalFailedPushes += result.failedPushes;
        totalFailedPops += result.failedPops;

        std::cout << name << " run " << (i + 1)
          << " -> " << result.opsPerSec << " msgs/sec"
          << " | failed pushes: " << result.failedPushes
          << " | failed pops: " << result.failedPops
          << '\n';
    }

    std::cout << name << " avg throughput: "
              << (totalOps / iterations) << " msgs/sec"
              << " | avg failed pushes: " << (totalFailedPushes / iterations)
              << " | avg failed pops: " << (totalFailedPops / iterations)
              << "\n\n";
}

int main()
{
    std::cout << "=== Throughput: Wait Strategy Comparison ===\n";

    run_throughput_benchmark<RingBuffer<int, 1024>>(
        "RingBuffer 1024 Spin",
        WaitStrategy::Spin
    );

    run_throughput_benchmark<RingBuffer<int, 1024>>(
        "RingBuffer 1024 Yield",
        WaitStrategy::Yield
    );

    run_throughput_benchmark<RingBuffer<int, 1024>>(
        "RingBuffer 1024 SpinThenYield",
        WaitStrategy::SpinThenYield
    );

    std::cout << "\n=== Latency: Wait Strategy Comparison ===\n";

    run_latency_benchmark<RingBuffer<TimedMessage, 1024>>(
        "RingBuffer 1024 Spin",
        WaitStrategy::Spin
    );

    run_latency_benchmark<RingBuffer<TimedMessage, 1024>>(
        "RingBuffer 1024 Yield",
        WaitStrategy::Yield
    );

    run_latency_benchmark<RingBuffer<TimedMessage, 1024>>(
        "RingBuffer 1024 SpinThenYield",
        WaitStrategy::SpinThenYield
    );

    return 0;
}