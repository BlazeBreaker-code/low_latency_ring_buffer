#include <iostream>
#include <thread>
#include <chrono>
#include <queue>
#include <mutex>

#include "ring_buffer.hpp"

template <typename T>
class MutexQueue {
public:
    bool try_push(const T& item) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(item);
        return true;
    }

    bool try_pop(T& item) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!queue_.empty()) {
            item = queue_.front();
            queue_.pop();
            return true;
        }
        return false;
    }

private:
    std::queue<T> queue_;
    std::mutex mutex_;
};

template <typename Queue>
double run_single_throughput_test() {
    const size_t N = 1'000'000;
    Queue queue;

    auto start = std::chrono::high_resolution_clock::now();
    std::thread producer([&]() {
        for (size_t i = 0; i < N; ++i) {
            while (!queue.try_push(static_cast<int>(i))) {
                // Busy wait until push is successful
            }
        }
    });

    std::thread consumer([&]() {
        size_t count = 0;
        int value;
        while (count < N) {
            if (queue.try_pop(value)) {
                ++count;
            }
        }
    });

    producer.join();
    consumer.join();

    auto end = std::chrono::high_resolution_clock::now();
    double seconds = std::chrono::duration<double>(end - start).count();
    
   return N / seconds; // Return throughput in items per second
}

template <typename Queue>
void run_throughput_test(const std::string& name, int iterations = 5) {
    double total = 0.0;
    for (int i = 0; i < iterations; ++i) {  
        double throughput = run_single_throughput_test<Queue>();
        std::cout << name << " run " << (i + 1) << " -> " << throughput << " ops/sec\n";
        total += throughput;
    }
    
    double average = total / iterations;
    std::cout << name << " average throughput: " << average << " ops/sec\n\n";
}

int main() {
    run_throughput_test<RingBuffer<int, 64>>("RingBuffer 64");
    run_throughput_test<RingBuffer<int, 256>>("RingBuffer 256");
    run_throughput_test<RingBuffer<int, 1024>>("RingBuffer 1024");
    run_throughput_test<RingBuffer<int, 8192>>("RingBuffer 8192");
    run_throughput_test<MutexQueue<int>>("MutexQueue");
    return 0;
}   