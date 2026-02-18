#include <coroutine>
#include <folly/init/Init.h>
#include <folly/experimental/coro/Task.h>
#include <folly/experimental/coro/Collect.h>
#include <folly/executors/StrandExecutor.h>
#include <folly/executors/ManualExecutor.h>
#include <folly/experimental/coro/BlockingWait.h>
#include <folly/experimental/coro/Sleep.h>
#include "benchcommon.hpp"
// #include "colib.h"
#include <iostream>

// Future-based benchmark similar to BM_cachecalc_boost_threadpool that runs max_iter tasks
void BM_cachecalc_folly_future_parallel(benchmark::State& state) {
    std::chrono::microseconds sleep_time = std::chrono::microseconds(state.range(0));
    auto globalEx = folly::getGlobalCPUExecutor();

    for (auto _ : state) {
        cachetype cache;
        auto strand_ = folly::StrandContext::create();
        // Create a strand executor to serialize access to the cache
        auto strandEx = folly::StrandExecutor::create(strand_, globalEx);

        std::vector<folly::Future<folly::Unit>> futures;
        futures.reserve(max_iter);

        for (int i = 0; i < max_iter; i++) {
            // 1. Initial jump to the strand for the first serialized calculation
            auto f = folly::via(strandEx.get(), [&cache] {
                calc(cache);
            })
            // 2. Perform non-blocking sleep (simulated IO)
            .thenValue([sleep_time](auto) {
                return folly::futures::sleep(sleep_time);
            })
            // 3. Chain execution back to the strand for the final calculation
            .via(strandEx.get())
            .thenValue([&cache](auto) {
                calc(cache);
            });

            futures.push_back(std::move(f));
        }

        // Synchronously wait for all 1,000 asynchronous chains to complete
        folly::collectAll(futures).wait();
        checkWork(state, cache);
    }
}

BENCHMARK(BM_cachecalc_folly_future_parallel)->Unit(benchmark::kMillisecond)->Range(8, 64);

// Coroutine-based benchmark similar to BM_cachecalc_boost_threadpool that runs max_iter tasks
void BM_cachecalc_folly_parallel(benchmark::State& state) {
    std::chrono::microseconds sleep_time = std::chrono::microseconds(state.range(0));
    
    for (auto _ : state) {
        cachetype cache;
        auto strand_ = folly::StrandContext::create();
        auto currentEx = folly::getGlobalCPUExecutor().get();
        auto strandExPtr = std::make_shared<folly::Executor::KeepAlive<>>(
            folly::StrandExecutor::create(strand_, currentEx));
        
        auto runTask = [&cache, sleep_time, strandExPtr]() -> folly::coro::Task<void> {
            // First calc with strand serialization
            co_await folly::coro::co_invoke([&cache]() -> folly::coro::Task<void> {
                calc(cache);
                co_return;
            }).scheduleOn(*strandExPtr);
            
            // Simulated IO
            co_await folly::coro::sleep(sleep_time);
            
            // Second calc with strand serialization
            co_await folly::coro::co_invoke([&cache]() -> folly::coro::Task<void> {
                calc(cache);
                co_return;
            }).scheduleOn(*strandExPtr);
        };
        
        auto runAllTasks = [&runTask]() -> folly::coro::Task<void> {
            std::vector<folly::coro::Task<void>> tasks;
            tasks.reserve(max_iter);
            
            for (int i = 0; i < max_iter; i++) {
                tasks.push_back(runTask());
            }
            
            co_await folly::coro::collectAllRange(std::move(tasks));
        };
        
        folly::coro::blockingWait(runAllTasks().scheduleOn(currentEx));
        checkWork(state, cache);
    }
}

BENCHMARK(BM_cachecalc_folly_parallel)->Unit(benchmark::kMillisecond)->Range(8, 64);

// BENCHMARK_MAIN();

int main(int argc, char** argv) {
    folly::Init init(&argc, &argv);
    ::benchmark::Initialize(&argc, argv);
    ::benchmark::RunSpecifiedBenchmarks();
    std::cout << "done\n";
    return 0;
}


