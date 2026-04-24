cmake_minimum_required(VERSION 3.16)
project(low_latency_ring_buffer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(ring_buffer_demo
    src/main.cpp
)

target_include_directories(ring_buffer_demo PRIVATE include)

add_executable(ring_buffer_benchmark
    benchmarks/benchmark.cpp
)

target_include_directories(ring_buffer_benchmark PRIVATE include)