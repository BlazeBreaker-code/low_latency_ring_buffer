#include <iostream>
#include <thread>
#include <chrono>
#include <queue>
#include <mutex>

#include "ring_buffer.hpp"

template<typename T>
class MutexQueue {
    public:
        using value_type = T;

        bool try_push(const T& item) {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push(item);
            return true;
        }

        bool try_pop(T& out) {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!queue_.empty()) {
                out = queue_.front(); queue_.pop();
                return true;
            }

            return false;
        }

    private:
        std::queue<T> queue_;
        std::mutex mutex_;
};

struct TimedMessage {
    using clock = std::chrono::high_resolution_clock;

    clock::time_point createdAt;
    std::size_t sequence;
};

template<typename QueueType>
double run_single_throughput_test() {
    using ValueType = typename QueueType::value_type;
    const std::size_t N = 1'000'000;
    QueueType queue;

    std::atomic<std::size_t> failedPushes{0};
    std::atomic<std::size_t> failedPops{0};

    auto start = std::chrono::high_resolution_clock::now();
    std::thread producer([&]() {
        for (std::size_t i = 0; i < N; ++i) {
            while (!queue.try_push(static_cast<ValueType>(i))) {
                // Busy waiting until push is successful
                std::this_thread::yield();
                failedPushes.fetch_add(1, std::memory_order_relaxed);
            }
        }
    });

    std::thread consumer([&]() {
        std::size_t count = 0;
        int value;
        while (count < N) {
            if (queue.try_pop(value)) {
                ++count;
            } else {
                std::this_thread::yield();
                failedPops.fetch_add(1, std::memory_order_relaxed);
            }
        }
    });

    producer.join();
    consumer.join();

    auto end = std::chrono::high_resolution_clock::now();
    double seconds = std::chrono::duration<double>(end - start).count();

    std::cout << "  failed pushes: " << failedPushes.load() << '\n';
    std::cout << "  failed pops:   " << failedPops.load() << '\n';

    return N / seconds;
}

template<typename QueueType>
void run_throughput_test(std::string name, int iterations = 5) {
    double total = 0.0;
    for (int i = 0; i < iterations; ++i) {
        double throughput = run_single_throughput_test<QueueType>();
        std::cout << name << " run " << (i + 1) << " -> " << throughput <<  " ops/sec\n";
        total += throughput; 
    }

    double average = total / iterations;
    std::cout << name << " avg throughput: " << average << " ops/sec\n\n";
}

template<typename QueueType>
void run_latency_test(const std::string& name) {
    using ValueType = typename QueueType::value_type;
    const std::size_t N = 1'000'000;

    QueueType queue;

    std::atomic<std::size_t> count{0};
    std::chrono::nanoseconds totalLatency{0};

    std::thread producer([&]() {
        for (std::size_t i = 0; i < N; ++i) {
            ValueType msg {
                ValueType::clock::now(),
                i
            };

            while (!queue.try_push(msg)) {
                std::this_thread::yield();
            }
        }
    });

    std::thread consumer([&]() {
        ValueType msg {};

        while (count < N) {
            if (queue.try_pop(msg)) {
                auto now = ValueType::clock::now();
                totalLatency += (now - msg.createdAt);
                ++count;
            } else {
                std::this_thread::yield();
            }
        }
    });

    producer.join();
    consumer.join();

    double avgLatencyNs = static_cast<double>(totalLatency.count() / N);
    std::cout << name << " avg latency: " << avgLatencyNs << " ns\n\n";
}

int main() {
    std::cout << "=== Throughput ===\n";

    run_throughput_test<RingBuffer<int, 1024>>("RingBuffer<int>");
    run_throughput_test<MutexQueue<int>>("MutexQueue<int>");

    std::cout << "\n=== Latency ===\n";

    run_latency_test<RingBuffer<TimedMessage, 1024>>("RingBuffer<Timed>");
    run_latency_test<MutexQueue<TimedMessage>>("MutexQueue<Timed>");
    return 0;
}