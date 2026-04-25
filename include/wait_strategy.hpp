#pragma once

#include <thread>

enum class WaitStrategy { Spin, Yield, SpinThenYield };

void apply_wait_strategy(WaitStrategy strategy, std::size_t& spin_count) {
    switch (strategy) {
        case WaitStrategy::Spin:
            return;

        case WaitStrategy::Yield:
            std::this_thread::yield();
            return;

        case WaitStrategy::SpinThenYield:
            if (++spin_count >= 100) {
                std::this_thread::yield();
                spin_count = 0;
            }
            return;
    }
}