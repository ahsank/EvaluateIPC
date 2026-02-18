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
* `BM_cachecalc_boost_coroutine`: Utilizes C++20 coroutines (`asio::awaitable`) to switch between the main pool and the strand for calculation and IO steps.


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
| `BM_cachecalc_boost_threadpool/8` | 1.91 ms | 0.051 ms | 12858 |
| `BM_cachecalc_boost_threadpool/64` | 1.89 ms | 0.047 ms | 10000 |
| `BM_cachecalc_boost_coroutine/8` | 4.68 ms | 1.42 ms | 489 |
| `BM_cachecalc_boost_coroutine/64` | 4.70 ms | 1.48 ms | 492 |

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
| `BM_cachecalc_mutex/8` | 6.57 ms | 0.078 ms | 1000 |
| `BM_cachecalc_mutex/64` | 12.7 ms | 0.082 ms | 1000 |
| `BM_cachecalc_async/8` | 10.2 ms | 6.47 ms | 102 |
| `BM_cachecalc_async/64` | 17.9 ms | 8.64 ms | 81 |
| `BM_cachecalc_simple_threadpool_class/8` | 7.33 ms | 3.16 ms | 216 |
| `BM_cachecalc_simple_threadpool_class/64` | 12.4 ms | 3.16 ms | 242 |

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
| `BM_cachecalc_boost_threadpool/8` | 2.56 ms | 0.107 ms | 1000 |
| `BM_cachecalc_boost_threadpool/64` | 2.36 ms | 0.101 ms | 6915 |
| `BM_cachecalc_boost_coroutine/8` | 4.14 ms | 0.716 ms | 959 |
| `BM_cachecalc_boost_coroutine/64` | 5.26 ms | 0.744 ms | 1016 |

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
| `BM_cachecalc_mutex/8` | 3.65 ms | 0.160 ms | 1000 |
| `BM_cachecalc_mutex/64` | 18.7 ms | 0.165 ms | 1000 |
| `BM_cachecalc_async/8` | 11.6 ms | 10.5 ms | 56 |
| `BM_cachecalc_async/64` | 24.2 ms | 11.9 ms | 56 |
| `BM_cachecalc_simple_threadpool_class/8` | 3.92 ms | 3.60 ms | 189 |
| `BM_cachecalc_simple_threadpool_class/64` | 22.5 ms | 20.3 ms | 33 |

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
