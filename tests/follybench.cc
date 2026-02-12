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

using StrandContext = folly::StrandContext;

folly::coro::Task<void> sleepTask(std::chrono::microseconds sleep_time) {
    co_await folly::coro::sleep(sleep_time);
    co_return;
}


template <class T>
class MyObject {
public:

    MyObject(std::chrono::microseconds sleeptime): sleep_time(sleeptime) {
    }
    
    folly::coro::Task<T> someMethod() {
        auto currentEx = co_await folly::coro::co_current_executor;
        auto strandEx = folly::StrandExecutor::create(strand_, currentEx);
        co_return co_await someMethodImpl().scheduleOn(strandEx);
    }

private:
    folly::coro::Task<T> someMethodImpl() {
         auto start = std::chrono::high_resolution_clock::now();
        // Safe to access otherState_ without further synchronisation.
        modify(otherState_);
        (void) co_await sleepTask(sleep_time);
        auto real_duration = duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now() - start);
        if (real_duration < sleep_time) {
            throw std::runtime_error("Sleep not working");
        }
        modify(otherState_);
        co_return otherState_;
    }

    void modify(int& val) {
        val++;
        calc(cache);
    }
    
    std::shared_ptr<StrandContext> strand_{StrandContext::create()};
    int otherState_{0};
    cachetype cache;
    std::chrono::microseconds sleep_time;
};


void BM_cachecalc_folly_threadpool(benchmark::State& state) {
    // std::cout << __cpp_impl_coroutine << "," << FOLLY_HAS_COROUTINES << std::endl;
    std::chrono::microseconds sleep_time = std::chrono::microseconds(state.range(0));
    MyObject<int> myobj(sleep_time);
    for (auto _ : state) {
        folly::coro::blockingWait(
            std::move(myobj.someMethod())
            .scheduleOn(folly::getGlobalCPUExecutor().get()));
    }
    
}
BENCHMARK(BM_cachecalc_folly_threadpool)->Unit(benchmark::kMillisecond)->Range(0, 64);

// Benchmark similar to BM_cachecalc_boost_threadpool that runs max_iter tasks
void BM_cachecalc_folly_parallel(benchmark::State& state) {
    std::chrono::microseconds sleep_time = std::chrono::microseconds(state.range(0));
    
    for (auto _ : state) {
        cachetype cache;
        auto strand_ = StrandContext::create();
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
            co_await sleepTask(sleep_time);
            
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


