# Lock-Free Ring Buffer Benchmarking (C++)

A high-performance single-producer single-consumer (SPSC) ring buffer implemented in C++, with a focus on **low-latency communication**, **contention behavior**, and **performance tradeoffs under different synchronization strategies**.

This project explores how design decisions such as wait strategies and buffer capacity impact **throughput, latency distribution, and contention** in concurrent systems.

---

# System Overview

```text
          Producer Thread
                |
                | try_push(message)
                v
      +----------------------+
      | Lock-Free RingBuffer |
      |                      |
      |  [ ][ ][ ][ ][ ][ ]  |
      |   ^              ^   |
      |  head           tail |
      +----------------------+
                |
                | try_pop(message)
                v
          Consumer Thread
```

The producer writes messages into the ring buffer using try_push, while the consumer reads messages using try_pop. The buffer uses atomic head/tail indices to coordinate between threads without a mutex.

Wait Strategy Under Contention

```text
try_push / try_pop fails
          |
          v
 +------------------+
 | Wait Strategy    |
 +------------------+
 | Spin             |  keep retrying immediately
 | Yield            |  give CPU back to scheduler
 | SpinThenYield    |  spin briefly, then yield
 +------------------+
```
 
---

# Features

### Lock-Free Ring Buffer (SPSC)
- Fixed-size circular buffer with atomic head/tail indices
- Wait-free for producer/consumer under non-contention conditions
- Cache-line aligned to reduce false sharing

### Configurable Wait Strategies
Supports multiple retry behaviors under contention:
- **Spin** (busy-wait)
- **Yield** (scheduler handoff)
- **Spin-Then-Yield** (hybrid strategy)

### Throughput Benchmarking
Measures:
- Messages per second
- Failed push/pop attempts (contention proxy)

### Latency Benchmarking
Captures end-to-end latency using timestamped messages:
- Average latency
- p50 / p95 / p99 / p99.9
- Max latency

### Capacity Sweep Analysis
Evaluates how buffer size affects:
- Throughput
- Latency distribution
- Backpressure and contention

---

# Example Output

## Wait Strategy Comparison

```text
=== Throughput: Wait Strategy Comparison ===
Benchmark                    msgs/s(M)       fail_push        fail_pop
----------------------------------------------------------------------
Spin                             17.17            2088         1719467
Yield                            24.04             477           57196
SpinThenYield                    19.77            1662         1186547

=== Latency: Wait Strategy Comparison ===
Benchmark                    msgs/s(M)     avg(ns)       p50       p95       p99       p99.9         max       fail_push        fail_pop
----------------------------------------------------------------------------------------------------------------------------------------
Spin                             11.02    92762.43  91441.60  98108.40 123666.60   193225.20   234341.80         1116163              55
Yield                            13.11    77623.62  75766.80  88949.60 109983.20   176575.00   189375.40          201993               0
SpinThenYield                    11.07    92333.90  91316.80 101749.80 112850.00   155141.40   160617.00         1061926              11
```

--- 

## Capacity Sweep (SpinThenYield)

```text
=== Throughput: Capacity Sweep (SpinThenYield) ===
Benchmark                    msgs/s(M)
--------------------------------------
64                               18.90
256                              19.26
1024                             19.90
8192                             20.14

=== Latency: Capacity Sweep (SpinThenYield) ===
Benchmark                    p99(us)
------------------------------------
64                                7.7
256                              29.7
1024                            112.7
8192                           1291.4
```

---

# Architecture

### RingBuffer
- Fixed-capacity circular buffer
- Uses atomic head/tail indices
- Employs memory ordering (acquire/release) for correctness
- Designed for SPSC usage

### Benchmark System
- Producer thread generates messages
- Consumer thread processes messages
- Configurable wait strategies control contention behavior
- Results aggregated and printed in structured tables

---

# Why SPSC Matters

This ring buffer is designed for single-producer single-consumer usage.

Only the producer writes to `tail`, and only the consumer writes to `head`. Both threads may read the other index, but neither contends on writes to the same index.

This ownership model keeps the implementation simpler and faster than a multi-producer multi-consumer queue, which requires additional coordination around shared writes, compare/exchange loops, and more complex contention handling.

---

# Memory Ordering

The ring buffer uses acquire/release memory ordering to safely publish messages between producer and consumer without a mutex.

Producer:
1. Writes the message into the buffer slot.
2. Release-stores the updated tail index.

Consumer:
1. Acquire-loads the tail index.
2. If data is available, reads the message.

The release/acquire pair guarantees that once the consumer observes the updated tail, it also observes the message written before that tail update.

---

# Thread Affinity

The benchmark includes thread affinity support to reduce scheduling noise during measurement.

Producer and consumer threads are assigned to separate cores when supported. This helps reduce thread migration, improves cache locality, and makes benchmark behavior more consistent.

On macOS, strict CPU pinning is not exposed in the same way as Linux, so the implementation uses an affinity hint rather than a hard guarantee.

In this benchmark, thread affinity exposed queueing behavior more clearly: with larger buffers, the producer can run ahead more consistently, causing messages to spend longer waiting in the queue.

---

# Design Decisions

### Fixed Capacity Buffer
Using a compile-time capacity:
- avoids dynamic allocation
- improves cache locality
- enables predictable performance

### Lock-Free Synchronization
Atomic operations eliminate:
- mutex overhead
- kernel scheduling cost

### Wait Strategy Abstraction
Separating retry behavior allows:
- controlled experimentation
- comparison of CPU vs latency tradeoffs

### Structured Benchmarking
Benchmarks are:
- repeatable
- isolated by variable (strategy vs capacity)
- reported with percentile metrics

---

# Key Insights

### Wait Strategy Tradeoffs
- **Spin**
  - lowest median and tail latency
  - highest contention and CPU usage
- **Yield**
  - highest throughput
  - large latency spikes due to scheduler interaction
- **Spin-Then-Yield**
  - balanced approach
  - reduces contention while maintaining reasonable latency

### Capacity Tradeoffs

- **Small buffers**
  - lower latency
  - higher contention
- **Large buffers**
  - higher throughput
  - increased latency due to queueing

> Larger buffers reduce backpressure but allow messages to wait longer before consumption, increasing tail latency.

---

# Build & Run

### Build


mkdir build
cd build
cmake ..
make


### Run


./ring_buffer_benchmark


---

# Project Structure


include/
ring_buffer.hpp
mutex_queue.hpp
wait_strategy.hpp
benchmark_math.hpp
benchmark_format.hpp

benchmarks/
benchmark.cpp

examples/
basic_usage.cpp


---

# Future Improvements

- Multi-producer / multi-consumer (MPMC) support
- NUMA-aware benchmarking
- Cache line padding experiments
- Lock-free memory reclamation strategies
- CPU affinity pinning for more stable measurements

---

# Motivation

Low-latency systems require careful balancing of:
- throughput
- latency
- contention
- CPU utilization

This project explores those tradeoffs through controlled experiments, providing insight into how synchronization strategies and buffer design impact real-world performance.
