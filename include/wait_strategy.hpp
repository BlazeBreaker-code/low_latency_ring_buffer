#pragma once

#include <thread>

enum class WaitStrategy { Spin, Yield, SpinThenYield, ExponentialBackoff };

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
        
        case WaitStrategy::ExponentialBackoff: 
            ++spin_count;

            if (spin_count < 10) return;

            if (spin_count < 100) {
                for (std::size_t i = 0; i < spin_count; ++i) {
                    // Intentional local spin
                }

                return;
            }

            std::this_thread::yield();
            spin_count = 0;
            return;
    }
}