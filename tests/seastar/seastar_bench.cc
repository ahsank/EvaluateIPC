#include <memory>
#include <mutex>
#include <stdlib.h> 
#include <seastar/core/app-template.hh>
#include <seastar/core/coroutine.hh>
#include <seastar/core/app-template.hh>
#include <seastar/core/thread.hh>
#include <seastar/core/do_with.hh>
#include <seastar/core/loop.hh>
#include <seastar/core/sleep.hh>
#include <seastar/util/later.hh>
#include <seastar/util/defer.hh>
#include <seastar/util/closeable.hh>
#include <benchmark/benchmark.h>
#include <boost/range/irange.hpp>
#include "../benchcommon.hpp"


static void BM_SeastarHash(benchmark::State& state) {
    int nshard = std::min(num_tasks, seastar::smp::count);
    std::chrono::microseconds sleep_time = std::chrono::microseconds(state.range(0));
    for (auto _ : state) {
        cachetype cache;
        seastar::parallel_for_each(boost::irange(0u, max_iter),
            [nshard, &cache, sleep_time] (unsigned f) {
                return seastar::smp::submit_to(0, [&cache] {
                    calc(cache);
                    return seastar::make_ready_future<>();
                }).then([sleep_time, f, nshard] {
                    return seastar::smp::submit_to(f % nshard, [sleep_time] {
                        
                        return seastar::sleep(sleep_time);
                    });
                }).then([&cache] {
                    return seastar::smp::submit_to(0, [&cache] {
                        calc(cache);
                        return seastar::make_ready_future<>();
                    });
                });
            }).get();
        checkWork(state, cache);
    }
}


BENCHMARK(BM_SeastarHash)->Unit(benchmark::kMillisecond)->Range(8, 64);

// BENCHMARK(BM_SeastarHash);


// BENCHMARK_MAIN();

int main(int argc, char** argv) {
    
    seastar::app_template app;
    app.run(argc, argv, [argc, argv] () {
        return seastar::async([&] {
            std::cout << "Running benchmarks...\n";
            int ac = argc;
            ::benchmark::Initialize(&ac, argv);
            ::benchmark::RunSpecifiedBenchmarks();
            std::cout << "done\n";
            // return seastar::make_ready_future<>();
        });
    });
}
