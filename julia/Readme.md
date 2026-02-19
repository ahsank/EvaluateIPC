# Julia Performance Tests

This directory contains Julia benchmarks that evaluate the performance of various concurrency patterns for the same "cache calc" workload as the C++ benchmarks.

## Overview

The Julia benchmarks test the same pattern as the C++ implementation:
- **1,000 iterations** of a task sequence
- Each task: **serialized compute** → **simulated IO (8-64µs)** → **serialized compute**
- Tests different threading/concurrency approaches

## Available Benchmarks

| Benchmark | Description |
| --- | --- |
| `cachecalc` | Single-threaded baseline: no parallelism |
| `cachecalc_mutex` | Single-threaded with mutex (same thread, no contention) |
| `cachecalc_threaded` | Multi-threaded using `Threads.@threads` (static work distribution) |
| `cachecalc_spawned` | Multi-threaded using `Threads.@spawn` (dynamic work distribution) |
| `cachecalc_optimized_spawned` | Multi-threaded with optimized calc using pre-computed lookup tables |

## Usage

```julia
julia> using BenchmarkTools
julia> include("./perftest/src/perftest.jl")
julia> using .perftest

# Single-threaded baseline
julia> @btime perftest.cachecalc(64)

# Mutex (control)
julia> @btime perftest.cachecalc_mutex(64)

# Multi-threaded (static scheduling)
julia> @btime perftest.cachecalc_threaded(64)

# Multi-threaded (dynamic scheduling)
julia> @btime perftest.cachecalc_spawned(64)

# Optimized multi-threaded
julia> @btime perftest.cachecalc_optimized_spawned(64)
```

## Benchmark Results (Julia 1.12.5, 4 Threads)

| Benchmark | Time | Allocations | Memory |
| --- | --- | --- | --- |
| `cachecalc` | 67.542 ms | 100,036 | 3.05 MiB |
| `cachecalc_mutex` | 67.676 ms | 100,039 | 3.05 MiB |
| `cachecalc_threaded` | 25.538 ms | 100,061 | 3.05 MiB |
| `cachecalc_spawned` | 5.831 ms | 105,055 | 3.41 MiB |
| `cachecalc_optimized_spawned` | 3.496 ms | 45,055 | 1.58 MiB |

## Key Observations

1. **Single-threaded overhead**: Mutex and baseline show identical times, indicating no contention overhead in Julia's mutex implementation.

2. **Static scheduling (@threads)**: Provides ~2.6x speedup over single-threaded with even distribution.

3. **Dynamic scheduling (@spawn)**: Achieves ~11.6x speedup by allowing the scheduler to overlap IO waits across all 1,000 tasks efficiently.

4. **Optimized compute**: Pre-computed lookup tables reduce allocations by 57% and memory usage by 54%, improving performance to ~3.5ms (19.3x speedup).

5. **Allocation pattern**: Task creation overhead is offset by the dramatic improvement in concurrent IO handling.

## Running the Benchmarks

```bash
cd julia/perftest
julia --project -t auto -e 'using BenchmarkTools; include("src/perftest.jl"); using .perftest; println("IO time = 64µs"); @btime perftest.cachecalc_spawned(64)'
```

## Notes

- Use `julia -t auto` to enable all available threads
- Use `Threads.nthreads()` to check the number of active threads
- Pre-allocated lookup tables in `calc_optimized` reduce parse/string overhead significantly
- The `fakeIO` function simulates blocking IO using `/dev/zero` reads to closely match C++ benchmark behavior
- Julia's dynamic task scheduling via `@spawn` achieves excellent performance on IO-bound workloads
