#include <coroutine>
#include <folly/init/Init.h>
#include <folly/experimental/coro/Task.h>
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
        // Other instances of someMethod() calls will be able to run
        // while this coroutine is suspended waiting for an RPC call
        // to complete.
        // co_await getClient()->co_someRpcCall();

       // When that call resumes, this coroutine is once again
       // executing with mutual exclusion.
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
BENCHMARK(BM_cachecalc_folly_threadpool)->Unit(benchmark::kMillisecond)->Range(0, 64);;

// BENCHMARK_MAIN();

int main(int argc, char** argv) {
    folly::Init init(&argc, &argv);
    ::benchmark::Initialize(&argc, argv);
    ::benchmark::RunSpecifiedBenchmarks();
    std::cout << "done\n";
    return 0;
}


