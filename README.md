# EvaluateIPC

This repository evaluates the performance and scalability of various C++ concurrency and Inter-Process Communication (IPC) techniques. It a set of benchmarks comparing the C++ Standard Library, Boost.Asio, Facebook Folly, and the Seastar framework.

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
* `BM_cachecalc_folly_parallel`: Leverages `folly::coro` and `StrandExecutor` to achieve high-performance asynchronous execution.


* **Seastar**:
* `BM_SeastarHash`: Serializes all cache access to Shard 0 using `smp::submit_to`, simulating a central coordinator.
* `BM_SeastarShardedCacheParallel`: A scalable, shared-nothing implementation using `seastar::sharded` where data is partitioned across all available cores, eliminating the central bottleneck.



---

## Benchmark Results (MacBook Pro Apple M2)

The following results were captured running inside a Docker container on a MacBook Pro (Apple M2, 4 shards assigned to Docker).

### Boost.Asio Benchmarks (`tests/boostbench`)

Boost demonstrates highly efficient scheduling, with threadpool versions performing better than coroutines in this specific workload.

| Benchmark | Time | CPU | Iterations |
| --- | --- | --- | --- |
| `BM_cachecalc_boost_threadpool/8` | 2.54 ms | 0.106 ms | 1000 |
| `BM_cachecalc_boost_threadpool/64` | 2.36 ms | 0.105 ms | 6962 |
| `BM_cachecalc_boost_coroutine/8` | 4.33 ms | 0.762 ms | 789 |
| `BM_cachecalc_boost_coroutine/64` | 5.32 ms | 0.767 ms | 916 |

### Standard Mutex & Async Benchmarks (`tests/mutexbench`)

The threadpool implementation significantly outperforms `std::async` by eliminating thread-spawning overhead.

| Benchmark | Time | CPU | Iterations |
| --- | --- | --- | --- |
| `BM_cachecalc_mutex/8` | 3.65 ms | 0.159 ms | 1000 |
| `BM_cachecalc_mutex/64` | 18.7 ms | 0.164 ms | 1000 |
| `BM_cachecalc_async/8` | 11.7 ms | 10.4 ms | 54 |
| `BM_cachecalc_async/64` | 25.8 ms | 13.7 ms | 57 |
| `BM_cachecalc_simple_threadpool_class/8` | 4.11 ms | 3.84 ms | 145 |
| `BM_cachecalc_simple_threadpool_class/64` | 21.9 ms | 19.1 ms | 36 |

### Seastar Framework Benchmarks (`tests/seastar/seastar_bench`)

Seastar achieves the lowest latency by bypassing kernel scheduling and utilizing a polling-based, shared-nothing architecture.

| Benchmark | Time | CPU | Iterations |
| --- | --- | --- | --- |
| `BM_SeastarHash/8` | 1.25 ms | 1.25 ms | 561 |
| `BM_SeastarHash/64` | 1.31 ms | 1.31 ms | 533 |
| **`BM_SeastarShardedCacheParallel/8`** | **0.827 ms** | 0.766 ms | 870 |
| **`BM_SeastarShardedCacheParallel/64`** | **0.871 ms** | 0.781 ms | 899 |

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
/build# ./tests/seastar/seastar_bench

```

### Local Development (Ubuntu)

Ensure dependencies like `libfast-float-dev`, `libgoogle-glog-dev`, and `liburing-dev` are installed:

```console
./docker/cmake/install-dependencies.sh
cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_FOLLY=ON
cmake --build build

```

## Debugging

Before debugging in GDB, disable verbose thread logs to keep the output readable:

```gdb
set print thread-events off

```
