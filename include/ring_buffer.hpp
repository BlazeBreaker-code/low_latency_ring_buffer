#pragma once

#include <array>
#include <atomic>
#include <cstddef>

template <typename T, std::size_t Capacity>
class RingBuffer {
    static_assert(Capacity > 1, "Capacity must be greater than 1");
public: 
    RingBuffer() = default;

    using value_type = T;

    bool try_push(const T& item);
    bool try_pop(T& out);

    bool empty() const;
    bool full() const;

    std::size_t size() const;
    static constexpr std::size_t capacity() {
        return Capacity - 1;
    }
    
private: 
    std::array<T, Capacity> buffer_{};

    alignas(64) std::atomic<std::size_t> head_{0};
    alignas(64) std::atomic<std::size_t> tail_{0};

    static constexpr std::size_t nextIndex (std::size_t index) {
        return (index + 1) % Capacity; // wrap around
    }
};

template <typename T, std::size_t Capacity>
bool RingBuffer<T, Capacity>::try_push(const T& item) {
    const std::size_t tail = tail_.load(std::memory_order_relaxed);
    const std::size_t nextTail = nextIndex(tail);

    if (nextTail == head_.load(std::memory_order_acquire)) {
        return false; // buffer full
    } 

    buffer_[tail] = item;
    tail_.store(nextTail, std::memory_order_release);
    return true;
}

template<typename T, std::size_t Capacity>
bool RingBuffer<T, Capacity>::try_pop(T& out) {
    const std::size_t head = head_.load(std::memory_order_relaxed);

    if (head == tail_.load(std::memory_order_acquire)) {
        return false; // buffer empty
    }

    out = buffer_[head];

    const std::size_t nextHead = nextIndex(head);
    head_.store(nextHead, std::memory_order_release);
    return true;
}

template<typename T, std::size_t Capacity>
std::size_t RingBuffer<T, Capacity>::size() const {
    const std::size_t head = head_.load(std::memory_order_acquire);
    const std::size_t tail = tail_.load(std::memory_order_acquire);

    return (tail + Capacity - head) % Capacity;
}

template<typename T, std::size_t Capacity>
bool RingBuffer<T, Capacity>::empty() const {
    return head_.load(std::memory_order_acquire) == tail_.load(std::memory_order_acquire);
}

template<typename T, std::size_t Capacity>
bool RingBuffer<T, Capacity>::full() const {
    const std::size_t head = head_.load(std::memory_order_acquire);
    const std::size_t tail = tail_.load(std::memory_order_acquire);

    return nextIndex(tail) == head;
}