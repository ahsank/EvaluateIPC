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

static void cache_calc(cachetype& cache, std::chrono::microseconds sleep_time) {
    for (int i=0; i < max_iter; i++) {
        calc(cache);
        fake_io(sleep_time);
        calc(cache);
    }
}

static void BM_cachecalc(benchmark::State& state) {
    for (auto _ : state) {
        cachetype cache;
        auto duration = std::chrono::microseconds(state.range(0));
        std::async(std::launch::async, [&cache, duration] {
            cache_calc(cache, duration);
        }).wait();
        checkWork(state, cache);
    }
}

BENCHMARK(BM_cachecalc)->Unit(benchmark::kMillisecond)->Range(0, 64);


static void cache_calc_mutex(cachetype& cache, std::mutex& m,
    std::chrono::microseconds sleep_time) {
    for (int i=0; i < max_iter; i++) {
        {
            std::scoped_lock lck(m);
            calc(cache);
        }
        fake_io(sleep_time);
        {
            std::scoped_lock lck(m);
            calc(cache);
        }
    }
}

static void BM_cachecalc_mutex(benchmark::State& state) {
    for (auto _ : state) {
        cachetype cache;
        std::mutex m;
        auto duration = std::chrono::microseconds(state.range(0));

        std::async(std::launch::async, [&] {
            cache_calc_mutex(cache, m, duration);
        }).wait();
        checkWork(state, cache);
    }
}

BENCHMARK(BM_cachecalc_mutex)->Unit(benchmark::kMillisecond)->Range(0,64);


static void cache_calc_async(cachetype& map,
    std::chrono::microseconds sleep_time) {
    int i = 0;
    std::queue<std::future<void>> futures;
    std::mutex m;

    for (int i=0; i < max_iter; i++) {
        futures.push(std::async([&]{
            {
                std::scoped_lock lck(m);
                calc(map);
            }
            fake_io(sleep_time);
            {
                std::scoped_lock lck(m);
                calc(map);
            }
        }));
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

static void BM_cachecalc_async(benchmark::State& state) {
    for (auto _ : state) {
        cachetype cache;
        cache_calc_async(cache,  std::chrono::microseconds(state.range(0)));
        checkWork(state, cache);
    }
}

BENCHMARK(BM_cachecalc_async)->Unit(benchmark::kMillisecond)->Range(8, 64);


BENCHMARK_MAIN();
