#pragma once

#include <thread>

#if defined(__linux__)
#include <pthread.h>
#include <sched.h>
#endif

#if defined(__APPLE__)
#include <mach/mach.h>
#include <mach/thread_policy.h>
#include <pthread.h>
#endif

inline void pin_thread_to_core([[maybe_unused]] int core_id)
{
#if defined(__linux__)
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);

    pthread_setaffinity_np(
        pthread_self(),
        sizeof(cpu_set_t),
        &cpuset
    );

#elif defined(__APPLE__)
    // macOS does not support strict CPU pinning like Linux.
    // THREAD_AFFINITY_POLICY is a scheduler hint that encourages
    // related threads with the same tag to stay near each other.
    thread_affinity_policy_data_t policy = {core_id + 1};

    thread_policy_set(
        mach_thread_self(),
        THREAD_AFFINITY_POLICY,
        reinterpret_cast<thread_policy_t>(&policy),
        THREAD_AFFINITY_POLICY_COUNT
    );

#else
    // Unsupported platform: no-op.
#endif
}