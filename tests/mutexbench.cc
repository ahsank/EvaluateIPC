#include <memory>
#include <mutex>
#include <array>
#include <benchmark/benchmark.h>
#include <iostream>
#include <ranges>
#include <execution>
#include <thread>
#include <future>
#include <sstream>
#include <math.h>
#include <iomanip>
#include <vector>
#include <queue>
#include <boost/lockfree/spsc_queue.hpp>
#include <boost/asio.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/asio/post.hpp>
#include <boost/thread/future.hpp>
#include <boost/bind/bind.hpp>
#include "benchcommon.hpp"

// This file contains benchmarks for various approaches to synchronizing access to a shared cache with simulated IO in between calculations. It includes:
// - A simple mutex-based approach (BM_cachecalc_mutex)
// - A parallelized version using std::for_each with a mutex (BM_cachecalc_mutex_par)
// - An async approach using std::async (BM_cachecalc_async)
// - A thread pool implementation with condition variables (BM_cachecalc_proper_threadpool)

// Measures the overhad of sleepting for a specified duration using std::this_thread::sleep_for
void BM_sleep(benchmark::State& state) {
    auto duration = std::chrono::microseconds(state.range(0));
    for (auto _ : state) {
        sleep_io::fake_io(duration);
    }
}

BENCHMARK(BM_sleep)->Unit(benchmark::kMicrosecond)->Range(8, 64);


// Measures the overhead of sleeping for a specified duration using /dev/zero reads
void BM_sleep_devzero(benchmark::State& state) {
    auto duration = std::chrono::microseconds(state.range(0));
    for (auto _ : state) {
        iotype::fake_io(duration);
    }
}
    
BENCHMARK(BM_sleep_devzero)->Unit(benchmark::kMicrosecond)->Range(8, 64);

// A single threaded benchmark that measures time spent on calculations only without any simulated IO.
// This serves as a baseline for the overhead of the calculations themselves without any synchronization or parallelism.
void cache_calc_only(cachetype& cache) {
    for (int i=0; i < max_iter; i++) {
        calc(cache);
        calc(cache);
    }
}

void BM_cachecalc_only(benchmark::State& state) {
    for (auto _ : state) {
        cachetype cache;
        cache_calc_only(cache);
        checkWork(state, cache);
    }
}

BENCHMARK(BM_cachecalc_only)->Unit(benchmark::kMillisecond);

// A single threaded benchmark that performs calculations, simulates IO, and then performs more calculations on a shared cache.
// This serves as a baseline for the overhead of the operations without any synchronization or parallelism.
void cache_calc(cachetype& cache, std::chrono::microseconds sleep_time) {
    for (int i=0; i < max_iter; i++) {
        calc(cache);
        iotype::fake_io(sleep_time);
        calc(cache);
    }
}

void BM_cachecalc(benchmark::State& state) {
    auto duration = std::chrono::microseconds(state.range(0));
    for (auto _ : state) {
        cachetype cache;
        cache_calc(cache, duration);
        checkWork(state, cache);
    }
}

BENCHMARK(BM_cachecalc)->Unit(benchmark::kMillisecond)->Range(8, 64);


// A multi-threaded benchmark that uses a mutex to synchronize access to the shared cache. 
// Each thread performs a portion of the total iterations, simulating IO outside the critical section to allow for some parallelism.
// The tasks are divided evenly among a fixed number of threads defined by 'num_tasks'.
void cache_calc_mutex(cachetype& cache, std::chrono::microseconds sleep_time) {
    std::mutex m;
    std::vector<std::thread> threads;
    // Divide iterations among threads
    int iters_per_thread = max_iter / num_tasks;

    auto worker = [&](int count) {
        for (int i = 0; i < count; ++i) {
            {
                std::scoped_lock lck(m); // Serialize access to shared cache
                calc(cache);
            }
            // Perform simulated IO outside the lock to allow parallelism
            iotype::fake_io(sleep_time); 
            {
                std::scoped_lock lck(m);
                calc(cache);
            }
        }
    };

    for (int i = 0; i < num_tasks; ++i) {
        threads.emplace_back(worker, iters_per_thread);
    }
    for (auto& t : threads) t.join();
}

void BM_cachecalc_mutex(benchmark::State& state) {
    auto duration = std::chrono::microseconds(state.range(0));
    for (auto _ : state) {
        cachetype cache;
        cache_calc_mutex(cache, duration);
        checkWork(state, cache);
    }
}

BENCHMARK(BM_cachecalc_mutex)->Unit(benchmark::kMillisecond)->Range(8,64);

// // A multi-threaded benchmark that uses std::for_each with the parallel execution policy to perform calculations in parallel while synchronizing access to the shared cache with a mutex.
// // Commenting as taking too long to run with the current parameters. Can be re-enabled with smaller iteration counts or by adjusting the workload.
// void cache_calc_mutex_par(cachetype& cache, std::chrono::microseconds sleep_time) {
//     std::mutex m;
//     std::vector<int> indices(max_iter);
//     std::iota(indices.begin(), indices.end(), 0);

//     std::for_each(std::execution::par, indices.begin(), indices.end(), [&](int) {
//         { std::scoped_lock lck(m); calc(cache); }
//         std::this_thread::sleep_for(sleep_time);
//         { std::scoped_lock lck(m); calc(cache); }
//     });
// }

// void BM_cachecalc_mutex_par(benchmark::State& state) {
//     auto duration = std::chrono::microseconds(state.range(0));
//     for (auto _ : state) {
//         cachetype cache;
//         cache_calc_mutex_par(cache, duration);
//         checkWork(state, cache);
//     }
// }

// BENCHMARK(BM_cachecalc_mutex_par)->Unit(benchmark::kMillisecond)->Range(8,64);

// A multi-threaded benchmark that uses std::async to perform calculations in parallel while synchronizing access to the shared cache with a mutex.
void cache_calc_async(cachetype& map,
    std::chrono::microseconds sleep_time) {
    int i = 0;
    std::queue<std::future<void>> futures;
    std::mutex m;

    for (int i=0; i < max_iter; i++) {
        // Each async task potentially can create a new thread so there is thread creation overhead
        futures.push(std::async([&]{
            {
                std::scoped_lock lck(m);
                calc(map);
            }
            iotype::fake_io(sleep_time);
            {
                std::scoped_lock lck(m);
                calc(map);
            }
        }));
        // Make sure no more parallel tasks than num_tasks
        while (futures.size() >= num_tasks) {
            futures.front().wait();
            futures.pop();
        }
    }
    while(!futures.empty()) {
        futures.front().wait();
        futures.pop();
    }
}

void BM_cachecalc_async(benchmark::State& state) {
    for (auto _ : state) {
        cachetype cache;
        cache_calc_async(cache,  std::chrono::microseconds(state.range(0)));
        checkWork(state, cache);
    }
}

BENCHMARK(BM_cachecalc_async)->Unit(benchmark::kMillisecond)->Range(8, 64);


// A multi-threaded benchmark that uses a custom thread pool implementation to perform calculations in parallel while synchronizing access to the shared cache with a mutex.
class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads) : stop(false) {
        for (size_t i = 0; i < num_threads; ++i) {
            workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex);
                        // Wait until there is a task or the pool is stopped
                        condition.wait(lock, [this] { 
                            return stop || !tasks.empty(); 
                        });
                        
                        if (stop && tasks.empty()) return;
                        
                        task = std::move(tasks.front());
                        tasks.pop();
                    }
                    task(); // Execute the task
                }
            });
        }
    }

    // Enqueue a new task into the pool
    template<class F>
    void enqueue(F&& f) {
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            tasks.emplace(std::forward<F>(f));
        }
        condition.notify_one();
    }

    ~ThreadPool() {
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            stop = true;
        }
        condition.notify_all();
        for (std::thread &worker : workers) {
            worker.join();
        }
    }

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
};

// A multi-threaded benchmark that uses a custom thread pool implementation to perform calculations in parallel while synchronizing access to the shared cache with a mutex.
//  Each task performs calculations on the shared cache, simulates IO, and then performs more calculations.
// The thread pool manages a fixed number of worker threads that execute the tasks from a queue, allowing for efficient reuse of threads and better control over concurrency compared to std::async.
void cache_calc_simple_threadpool_class(cachetype& map, std::chrono::microseconds sleep_time) {
    std::atomic<int> completed_tasks{0};
    std::mutex m; // Mutex for serialized cache access
    
    // Instantiate the reusable thread pool with the defined number of tasks
    ThreadPool pool(num_tasks);

    // Dispatch tasks to the pool
    for (int i = 0; i < max_iter; i++) {
        pool.enqueue([&, sleep_time] {
            {
                std::scoped_lock lck(m);
                calc(map);
            }
            iotype::fake_io(sleep_time);
            {
                std::scoped_lock lck(m);
                calc(map);
            }
            completed_tasks.fetch_add(1, std::memory_order_relaxed);
        });
    }

    // Wait for all tasks to complete as in the original implementation
    while (completed_tasks.load(std::memory_order_relaxed) < max_iter) {
        std::this_thread::yield();
    }
}

void BM_cachecalc_simple_threadpool_class(benchmark::State& state) {
    for (auto _ : state) {
        cachetype cache;
        cache_calc_simple_threadpool_class(cache, std::chrono::microseconds(state.range(0)));
        checkWork(state, cache);
    }
}

BENCHMARK(BM_cachecalc_simple_threadpool_class)->Unit(benchmark::kMillisecond)->Range(8, 64);

BENCHMARK_MAIN();
