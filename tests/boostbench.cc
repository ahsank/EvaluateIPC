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



static void BM_cachecalc_threadpool(benchmark::State& state) {
    boost::asio::thread_pool pool(num_tasks);
    boost::asio::thread_pool calcpool(1);
    auto strand_ = make_strand(calcpool.get_executor());
  
    for (auto _ : state) {
        cachetype cache;
        std::vector<boost::unique_future<void> > futures;
        futures.reserve(max_iter);
        auto fn = [&] {
            fake_io();
            auto f = boost::asio::post(strand_, std::packaged_task<void()>([&] {
                calc(cache);
            }));
            f.wait();
        };
        for (int i=0; i < max_iter; i++) {
            boost::packaged_task<void> task(fn);
            futures.push_back(task.get_future());
            boost::asio::post(pool, std::move(task));
        }
        //for (auto& f: futures) {
        //    f.wait();
        //}
        boost::wait_for_all(futures.begin(), futures.end());
        checkWork(state, cache);
    }
}

BENCHMARK(BM_cachecalc_threadpool)->Unit(benchmark::kMillisecond);


BENCHMARK_MAIN();

