#define BOOST_THREAD_PROVIDES_FUTURE
#define BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION
#define BOOST_THREAD_VERSION 4

#include <memory>
#include <mutex>
#include <array>
#include <iostream>
#include <ranges>
#include <execution>
#include <future>
#include <sstream>
#include <math.h>
#include <iomanip>
#include <vector>
#include <queue>
#include <semaphore>
#include <boost/lockfree/spsc_queue.hpp>
#include <boost/asio.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/thread/future.hpp>
#include <boost/bind/bind.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/steady_timer.hpp>
#include "benchcommon.hpp"

// Multi threaded benchmark that uses Boost Asio's thread pool to parallize calculations while synchronizing access to the shared cache with a strand. 
// Each task performs calculations on the shared cache, simulates IO with an asynchronous timer, and then performs more calculations. 
// The strand ensures that the calculations on the shared cache are serialized while allowing the simulated IO to run in parallel without blocking other tasks from starting their IO.
void BM_cachecalc_boost_threadpool(benchmark::State& state) {
    boost::asio::thread_pool pool(num_tasks);
    boost::asio::thread_pool calcpool(1);
    auto strand_ = boost::asio::make_strand(calcpool.get_executor());
    std::chrono::microseconds sleep_time(state.range(0));
    
    for (auto _ : state) {
        cachetype cache;
        std::atomic<int> completed{0};
        std::promise<void> all_done;
        auto done_future = all_done.get_future();

        for (int i = 0; i < max_iter; i++) {
            // Step 1: Initial calculation on the strand
            boost::asio::post(strand_, [&, sleep_time]() {
                calc(cache);

                // Step 2: Non-blocking simulated IO
                auto timer = std::make_shared<boost::asio::steady_timer>(pool.get_executor(), sleep_time);
                timer->async_wait([&, timer](auto ec) {
                    // Step 3: Final calculation back on the strand
                    boost::asio::post(strand_, [&]() {
                        calc(cache);
                        // Signal completion of the entire batch
                        if (completed.fetch_add(1, std::memory_order_relaxed) + 1 == max_iter) {
                            all_done.set_value();
                        }
                    });
                });
            });
        }
        done_future.wait();
        checkWork(state, cache);
    }
}

BENCHMARK(BM_cachecalc_boost_threadpool)->Unit(benchmark::kMillisecond)
->Range(8, 64);

// A multi-threaded benchmark that uses Boost Asio's coroutine support to parallize calculations while synchronizing access to the shared cache with a strand.
void BM_cachecalc_boost_coroutine(benchmark::State& state) {
    std::chrono::microseconds sleep_time(state.range(0));

    // 1. Move pools outside the measurement loop to avoid creation/join overhead.
    boost::asio::thread_pool pool(num_tasks);
    auto main_executor = pool.get_executor();
    auto strand_ = boost::asio::make_strand(main_executor);

    for (auto _ : state) {
        cachetype cache;
        std::atomic<int> completed{0};
        std::promise<void> all_done;
        auto done_future = all_done.get_future();

        auto task_coro = [&]() -> boost::asio::awaitable<void> {
            // Switch to strand: resume on strand executor, then run calc
            co_await boost::asio::post(strand_, boost::asio::use_awaitable);
            calc(cache);

            // Switch back to pool executor for IO
            // Return to main executor for IO to avoid serializing the wait
            co_await boost::asio::dispatch(
                boost::asio::bind_executor(main_executor, boost::asio::use_awaitable)
            );
            // Use an individual timer to simulate independent IO tasks
            boost::asio::steady_timer individual_timer(main_executor, sleep_time);
            co_await individual_timer.async_wait(boost::asio::use_awaitable);

            // Re-enter strand for the final serialized calculation
            co_await boost::asio::post(strand_, boost::asio::use_awaitable);
            calc(cache);

            if (completed.fetch_add(1, std::memory_order_relaxed) + 1 == max_iter) {
                all_done.set_value();
            }
            co_return;
        };

        for (int i = 0; i < max_iter; i++) {
            boost::asio::co_spawn(main_executor, task_coro(), boost::asio::detached);
        }

        done_future.wait();
        checkWork(state, cache);
    }
    
    pool.stop();
    pool.join();
}

BENCHMARK(BM_cachecalc_boost_coroutine)->Unit(benchmark::kMillisecond)
->Range(8, 64);


BENCHMARK_MAIN();

