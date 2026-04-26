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
Spin                             23.09           24694           28191
Yield                            25.00              53             175
SpinThenYield                    24.01            5603            9411

=== Latency: Wait Strategy Comparison ===
Benchmark                    msgs/s(M)     avg(ns)       p50       p95       p99       p99.9         max
----------------------------------------------------------------------------------------------------------
Spin                             17.77     1823.11    125.00  12875.40  26883.40    54758.40    72708.40
Yield                            19.70     8362.55    358.40  28099.60  44842.00  2075558.40  2091758.40
SpinThenYield                    17.14     8220.30    733.40  38841.40  71441.60   105408.20   145666.80
```

--- 

## Capacity Sweep (SpinThenYield)

```text
=== Throughput: Capacity Sweep (SpinThenYield) ===
Benchmark                    msgs/s(M)
--------------------------------------
64                               24.68
256                              22.46
1024                             24.14
8192                             24.54

=== Latency: Capacity Sweep (SpinThenYield) ===
Benchmark                    p99(us)
------------------------------------
64                                2.4
256                               8.5
1024                             23.7
8192                             30.7
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
