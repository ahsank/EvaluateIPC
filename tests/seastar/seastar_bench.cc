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
#include <seastar/core/sharded.hh>
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


BENCHMARK(BM_SeastarHash)->Unit(benchmark::kMillisecond)->Range(0, 64);

// Service to manage partitioned cache data on each shard
class cache_service {
public:
    cachetype cache;
    
    // Perform calculations on the local shard's cache
    seastar::future<> do_calc() {
        calc(cache);
        return seastar::make_ready_future<>();
    }

    // Verification helper
    void verify(benchmark::State& state, unsigned iters_per_shard) {
        checkWork(state, cache, iters_per_shard);
    }
};

static void BM_SeastarShardedCacheParallel(benchmark::State& state) {
    std::chrono::microseconds sleep_time = std::chrono::microseconds(state.range(0));
    unsigned n_shards = seastar::smp::count;
    unsigned iters_per_shard = max_iter / n_shards;

    for (auto _ : state) {
        seastar::sharded<cache_service> sharded_cache;
        
        sharded_cache.start().then([&sharded_cache, sleep_time, iters_per_shard] {
            // Distribute work across all shards
            return seastar::parallel_for_each(boost::irange(0u, seastar::smp::count), 
                [&sharded_cache, sleep_time, iters_per_shard](unsigned shard_id) {
                    return seastar::smp::submit_to(shard_id, [&sharded_cache, sleep_time, iters_per_shard] {
                        // FIX: Use parallel_for_each to allow the reactor to handle multiple sleeps concurrently
                        return seastar::parallel_for_each(boost::irange(0u, iters_per_shard), 
                            [&sharded_cache, sleep_time](unsigned) {
                                return sharded_cache.local().do_calc().then([sleep_time] {
                                    return seastar::sleep(sleep_time);
                                }).then([&sharded_cache] {
                                    return sharded_cache.local().do_calc();
                                });
                            });
                    });
                });
        }).then([&sharded_cache, &state, iters_per_shard] {
            return sharded_cache.invoke_on_all([&state, iters_per_shard](cache_service& local_svc) {
                local_svc.verify(state, iters_per_shard);
            });
        }).then([&sharded_cache] {
            return sharded_cache.stop();
        }).get();
    }
}

BENCHMARK(BM_SeastarShardedCacheParallel)->Unit(benchmark::kMillisecond)->Range(8, 64);


int main(int argc, char** argv) {
    seastar::app_template app;
    app.run(argc, argv, [argc, argv] () {
        return seastar::async([&] {
            std::cout << "Running benchmarks...\n";
            int ac = argc;
            ::benchmark::Initialize(&ac, argv);
            ::benchmark::RunSpecifiedBenchmarks();
            std::cout << "done\n";
        });
    });
}
