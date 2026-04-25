#include <iostream>

#include "ring_buffer.hpp"

int main() {
    RingBuffer<int, 4> queue;

    std::cout << "Initial -> size: " << queue.size() << ", empty: " << queue.empty()
              << ", full: " << queue.full() << '\n';

    queue.try_push(1);
    queue.try_push(2);
    queue.try_push(3);

    std::cout << "After pushes -> size: " << queue.size() << ", capacity: " << queue.capacity()
              << ", empty: " << queue.empty() << ", full: " << queue.full() << '\n';

    int value = 0;
    queue.try_pop(value);
    queue.try_pop(value);

    std::cout << "After 2 pops -> size: " << queue.size() << ", empty: " << queue.empty()
              << ", full: " << queue.full() << '\n';

    queue.try_push(4);
    queue.try_push(5);

    std::cout << "After wrap pushes -> size: " << queue.size() << ", full: " << queue.full()
              << '\n';

    while (queue.try_pop(value)) {
        std::cout << "Popped: " << value << '\n';
    }

    std::cout << "Final -> size: " << queue.size() << ", empty: " << queue.empty()
              << ", full: " << queue.full() << '\n';

    return 0;
}