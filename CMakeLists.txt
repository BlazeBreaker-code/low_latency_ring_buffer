cmake_minimum_required(VERSION 3.16)

project(low_latency_ring_buffer
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Main benchmark target
add_executable(ring_buffer_benchmark
    benchmarks/benchmark.cpp
)

target_include_directories(ring_buffer_benchmark
    PRIVATE
        include
)

target_compile_options(ring_buffer_benchmark
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)

# Optional basic usage example
add_executable(ring_buffer_example
    examples/basic_usage.cpp
)

target_include_directories(ring_buffer_example
    PRIVATE
        include
)

target_compile_options(ring_buffer_example
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)