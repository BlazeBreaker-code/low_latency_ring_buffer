#pragma once

#include <array>
#include <atomic>
#include <cstddef>

template <typename T, std::size_t Capacity>
class RingBuffer {
    static_assert(Capacity > 0, "Capacity must be greater than 0");

public:
    RingBuffer() = default;
    bool try_push(const T& item);
    bool try_pop(T& out);

    bool empty() const;
    bool full() const;
    std::size_t size() const;
    static constexpr std::size_t capacity() { return Capacity - 1; }

private:    
    std::array<T, Capacity> buffer_;

    alignas(64) std::atomic<std::size_t> head_{0};
    alignas(64) std::atomic<std::size_t> tail_{0};

    static constexpr std::size_t next_index(std::size_t index) {
        return (index + 1) % Capacity;
    }
};

template <typename T, std::size_t Capacity>
bool RingBuffer<T, Capacity>::try_push(const T& item) {
    std::size_t tail = tail_.load(std::memory_order_relaxed);
    std::size_t next_tail = next_index(tail);
    
    if (next_tail == head_.load(std::memory_order_acquire)) {
        return false; // Buffer is full
    }

    buffer_[tail] = item;
    tail_.store(next_tail, std::memory_order_release);
    return true;
}

template <typename T, std::size_t Capacity>
bool RingBuffer<T, Capacity>::try_pop(T& out) {
    std::size_t head = head_.load(std::memory_order_relaxed);
    if (head == tail_.load(std::memory_order_acquire)) {
        return false; // Buffer is empty
    }

    out = buffer_[head];
    head_.store(next_index(head), std::memory_order_release);
    return true;
}

template <typename T, std::size_t Capacity>
bool RingBuffer<T, Capacity>::empty() const {
    return head_.load(std::memory_order_acquire) == tail_.load(std::memory_order_acquire);
}

template <typename T, std::size_t Capacity>
bool RingBuffer<T, Capacity>::full() const {
    const auto tail = tail_.load(std::memory_order_acquire);
    const auto next = next_index(tail);
    return next == head_.load(std::memory_order_acquire);
}

template <typename T, std::size_t Capacity>
std::size_t RingBuffer<T, Capacity>::size() const {
    const auto head = head_.load(std::memory_order_acquire);
    const auto tail = tail_.load(std::memory_order_acquire);
    return (tail + Capacity - head) % Capacity;
}