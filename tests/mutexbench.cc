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

static void cache_calc(cachetype& cache) {
    for (int i=0; i < max_iter; i++) {
        fake_io();
        calc(cache);
    }
}

static void BM_cachecalc(benchmark::State& state) {
    for (auto _ : state) {
        cachetype cache;
        cache_calc(cache);
        checkWork(state, cache);
    }
}

BENCHMARK(BM_cachecalc)->Unit(benchmark::kMillisecond);


static void cache_calc_async(cachetype& map) {
    int i = 0;
    std::vector<std::future<void>> futures;
    std::mutex m;

    for (int i=0; i < max_iter; i++) {
        futures.push_back(std::async([&]{
            fake_io();
            {
                std::scoped_lock lck(m);
                calc(map);
            }
        }));
        if (futures.size() >= num_tasks) {
            for (auto& f : futures) {
                f.wait();
            }
            futures.clear();
        }
    }
    for (auto& f : futures) {
        f.wait();
    }
}

static void BM_cachecalc_async(benchmark::State& state) {
    for (auto _ : state) {
        cachetype cache;
        cache_calc_async(cache);
        checkWork(state, cache);
    }
}

BENCHMARK(BM_cachecalc_async)->Unit(benchmark::kMillisecond);


BENCHMARK_MAIN();

