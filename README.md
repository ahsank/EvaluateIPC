Here is the updated `README.md` content, incorporating your latest benchmark results for both the native macOS environment and the Linux Docker container.

---

# EvaluateIPC

This repository evaluates the performance and scalability of various C++ concurrency and Inter-Process Communication (IPC) techniques. It provides a set of benchmarks comparing the C++ Standard Library, Boost.Asio, Facebook Folly, and the Seastar framework.

## Benchmark Intent: The "Cache Calc" Pattern

The primary benchmark in this project simulates a high-concurrency scenario common in server-side applications: **serialized computation interleaved with asynchronous IO waits**.

### The Workflow

Each benchmark iteration executes 1,000 tasks (default `max_iter`) with a concurrency level of 8 tasks (`num_tasks`). Each task follows this pattern:

1. **Serialized Compute**: Perform a calculation on a shared resource (an `unordered_map` cache) protected by a synchronization primitive such as a mutex, strand, or shard logic.
2. **Simulated IO**: Perform a non-blocking delay (8µs to 64µs) using asynchronous timers.
3. **Serialized Compute**: Perform a second calculation on the same shared resource.

### Why this Pattern?

This pattern tests the efficiency of a framework's **context-switching** and **task-scheduling** capabilities. A high-performance framework should be able to overlap the "IO" periods of all 1,000 tasks, effectively parallelizing the wait time and ensuring the total benchmark time is dominated only by the serialized compute portions and framework overhead.

---

## Implementation Details

* **C++ Standard Library**:
* `BM_cachecalc_mutex`: Uses manual `std::thread` management and a shared `std::mutex`.
* `BM_cachecalc_async`: Uses `std::async` to launch tasks, highlighting the overhead of thread creation and destruction.
* `BM_cachecalc_simple_threadpool_class`: Uses a custom `ThreadPool` class to manage worker lifecycles via condition variables.


* **Boost.Asio**:
* `BM_cachecalc_boost_threadpool`: Uses `asio::thread_pool` and `asio::strand` to serialize access to the cache map.
* `BM_cachecalc_boost_coroutine`: Uses C++20 coroutines (`asio::awaitable`) to switch between the main pool and the strand for calculation and IO steps.
* `BM_cachecalc_boost_coroutine_strand`: Runs both compute and simulated IO on the strand to reduce thread context switching.
* `BM_cachecalc_boost_coroutine_io_context`: Runs everything on a single-thread `io_context` (no thread pool).


* **Facebook Folly**:
* `BM_cachecalc_folly_future_parallel`: Replicates the logic using `folly::Future` and `StrandExecutor` to chain asynchronous tasks.
* `BM_cachecalc_folly_parallel`: Leverages `folly::coro` and `StrandExecutor` to achieve high-performance asynchronous execution.


* **Seastar**:
* `BM_SeastarHash`: Serializes all cache access to Shard 0 using `smp::submit_to`, simulating a central coordinator.
* `BM_SeastarShardedCacheParallel`: A scalable, shared-nothing implementation using `seastar::sharded` where data is partitioned across all available cores, eliminating the central bottleneck.



---

## Benchmark Results (MacBook Pro Apple M2)

The following results were captured on a MacBook Pro (Apple M2).

### Native macOS (8 Cores)

These results show the overhead in a native environment without containerization.

#### Boost.Asio (`tests/boostbench`)

| Benchmark | Time | CPU | Iterations |
| --- | --- | --- | --- |
| `BM_cachecalc_boost_threadpool/8` | 2.17 ms | 0.050 ms | 10000 |
| `BM_cachecalc_boost_threadpool/64` | 2.06 ms | 0.046 ms | 10000 |
| `BM_cachecalc_boost_coroutine/8` | 2.93 ms | 1.32 ms | 631 |
| `BM_cachecalc_boost_coroutine/64` | 2.82 ms | 1.17 ms | 572 |
| `BM_cachecalc_boost_coroutine_strand/8` | 2.11 ms | 0.154 ms | 4584 |
| `BM_cachecalc_boost_coroutine_strand/64` | 2.09 ms | 0.153 ms | 4548 |
| `BM_cachecalc_boost_coroutine_io_context/8` | 2.03 ms | 1.99 ms | 358 |
| `BM_cachecalc_boost_coroutine_io_context/64` | 1.98 ms | 1.95 ms | 350 |

#### Facebook Folly (`tests/follybench`)

| Benchmark | Time | CPU | Iterations |
| --- | --- | --- | --- |
| `BM_cachecalc_folly_future_parallel/8` | 2.53 ms | 0.226 ms | 1000 |
| `BM_cachecalc_folly_future_parallel/64` | 2.48 ms | 0.226 ms | 3098 |
| `BM_cachecalc_folly_parallel/8` | 2.90 ms | 0.006 ms | 1000 |
| `BM_cachecalc_folly_parallel/64` | 2.93 ms | 0.006 ms | 1000 |

#### Standard Mutex & Async (`tests/mutexbench`)

| Benchmark | Time | CPU | Iterations |
| --- | --- | --- | --- |
| `BM_sleep/8` | 13.0 us | 1.73 us | 415551 |
| `BM_sleep/64` | 85.0 us | 2.96 us | 100000 |
| `BM_sleep_devzero/8` | 11.5 us | 11.5 us | 60083 |
| `BM_sleep_devzero/64` | 66.3 us | 66.2 us | 10573 |
| `BM_cachecalc_only` | 1.38 ms | 1.38 ms | 512 |
| `BM_cachecalc/8` | 13.1 ms | 13.1 ms | 53 |
| `BM_cachecalc/64` | 67.7 ms | 67.6 ms | 10 |
| `BM_cachecalc_mutex/8` | 7.30 ms | 0.091 ms | 1000 |
| `BM_cachecalc_mutex/64` | 13.7 ms | 0.099 ms | 1000 |
| `BM_cachecalc_async/8` | 11.3 ms | 7.01 ms | 92 |
| `BM_cachecalc_async/64` | 19.0 ms | 9.10 ms | 76 |
| `BM_cachecalc_simple_threadpool_class/8` | 8.46 ms | 3.78 ms | 192 |
| `BM_cachecalc_simple_threadpool_class/64` | 13.1 ms | 3.34 ms | 211 |

### Docker Container (Linux, 4 Cores)

These results show the performance characteristics when constrained to 4 shards within a Docker container.

#### Seastar Framework (`tests/seastar/seastar_bench`)

| Benchmark | Time | CPU | Iterations |
| --- | --- | --- | --- |
| `BM_SeastarHash/8` | 1.25 ms | 1.21 ms | 586 |
| `BM_SeastarHash/64` | 1.31 ms | 1.27 ms | 569 |
| **`BM_SeastarShardedCacheParallel/8`** | **0.759 ms** | 0.720 ms | 999 |
| **`BM_SeastarShardedCacheParallel/64`** | **0.773 ms** | 0.736 ms | 957 |

#### Boost.Asio (`tests/boostbench`)

| Benchmark | Time | CPU | Iterations |
| --- | --- | --- | --- |
| `BM_cachecalc_boost_threadpool/8` | 2.58 ms | 0.108 ms | 1000 |
| `BM_cachecalc_boost_threadpool/64` | 2.33 ms | 0.102 ms | 6775 |
| `BM_cachecalc_boost_coroutine/8` | 3.32 ms | 0.775 ms | 908 |
| `BM_cachecalc_boost_coroutine/64` | 3.31 ms | 0.797 ms | 975 |
| `BM_cachecalc_boost_coroutine_strand/8` | 1.83 ms | 0.239 ms | 2861 |
| `BM_cachecalc_boost_coroutine_strand/64` | 1.84 ms | 0.240 ms | 2966 |
| `BM_cachecalc_boost_coroutine_io_context/8` | 1.68 ms | 1.68 ms | 400 |
| `BM_cachecalc_boost_coroutine_io_context/64` | 1.68 ms | 1.68 ms | 419 |

#### Facebook Folly (`tests/follybench`)

| Benchmark | Time | CPU | Iterations |
| --- | --- | --- | --- |
| `BM_cachecalc_folly_future_parallel/8` | 2.36 ms | 0.450 ms | 1549 |
| `BM_cachecalc_folly_future_parallel/64` | 2.34 ms | 0.455 ms | 1533 |
| `BM_cachecalc_folly_parallel/8` | 3.00 ms | 0.027 ms | 1000 |
| `BM_cachecalc_folly_parallel/64` | 2.95 ms | 0.028 ms | 1000 |

#### Standard Mutex & Async (`tests/mutexbench`)

| Benchmark | Time | CPU | Iterations |
| --- | --- | --- | --- |
| `BM_sleep/8` | 86.0 us | 5.52 us | 128021 |
| `BM_sleep/64` | 155 us | 5.78 us | 126059 |
| `BM_sleep_devzero/8` | 9.03 us | 8.93 us | 75674 |
| `BM_sleep_devzero/64` | 65.2 us | 65.2 us | 10784 |
| `BM_cachecalc_only` | 1.03 ms | 1.03 ms | 673 |
| `BM_cachecalc/8` | 10.1 ms | 10.1 ms | 68 |
| `BM_cachecalc/64` | 66.2 ms | 66.1 ms | 11 |
| `BM_cachecalc_mutex/8` | 4.50 ms | 0.250 ms | 1000 |
| `BM_cachecalc_mutex/64` | 21.0 ms | 0.334 ms | 1000 |
| `BM_cachecalc_async/8` | 14.5 ms | 12.2 ms | 48 |
| `BM_cachecalc_async/64` | 30.9 ms | 15.7 ms | 51 |
| `BM_cachecalc_simple_threadpool_class/8` | 5.91 ms | 5.01 ms | 150 |
| `BM_cachecalc_simple_threadpool_class/64` | 23.7 ms | 20.0 ms | 30 |

---

## How to Run

### Using Docker

From the root of this repository:

```console
./bin/rundocker --build
./bin/rundocker

```

Inside the container:

```console
/build# /workspace/bin/runcmakeindocker.sh
/build# make
/build# ./tests/mutexbench
/build# ./tests/boostbench
/build# ./tests/follybench
/build# ./tests/seastar/seastar_bench

```

### Local Development (Ubuntu/OSX)

Ensure dependencies like `libfast-float-dev`, `libgoogle-glog-dev`, and `liburing-dev` are installed:

```console
cd /workspace
./docker/cmake/install-dependencies.sh
cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_FOLLY=ON
cmake --build build

```

## Debugging

Before debugging in GDB, disable verbose thread logs to keep the output readable:

```gdb
set print thread-events off

```
