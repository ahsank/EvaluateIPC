#define BOOST_THREAD_PROVIDES_FUTURE
#define BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION
#define BOOST_THREAD_VERSION 4

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
#include <boost/asio/deadline_timer.hpp>
#include "benchcommon.hpp"

constexpr int qsize = 100;
template <class T>
class cond_queue {
public:
    std::vector<T> buff;
    int pos = 0;
    volatile bool closed = false;
    std::mutex m_mutex;
    std::condition_variable m_cv;

    cond_queue(): buff(qsize) {}
    void add(T val) {
        {
            std::unique_lock<std::mutex> the_lock(m_mutex);
            while (pos >= qsize) {
                m_cv.wait(the_lock, [this] {
                    return pos < qsize;
                });
            }
            buff[pos++] = std::move(val);
        }
        m_cv.notify_one();
    }

    void close() {
        std::unique_lock<std::mutex> the_lock(m_mutex);
        closed = true;
        m_cv.notify_all();
    }
    
    std::pair<T, bool> get() {
        T val;
        bool is_closed;
        {
            std::unique_lock<std::mutex> the_lock(m_mutex);
            while ( pos <= 0 && !closed) {
                m_cv.wait(the_lock, [this] {
                    return pos > 0 || closed;
                });
            }
            is_closed = closed;
            if (pos > 0) {
                val = std::move(buff[--pos]);
            } 
        }
        m_cv.notify_one();
        return std::make_pair(std::move(val), is_closed);
    }
};

class CacheActor {
public:
    cachetype cache;
    using tasktype = std::packaged_task<void(cachetype&)>;
    cond_queue<tasktype> cqueue;
    std::thread actor_thread;

    void start() {
        auto actor_thread = std::thread([&] {
            while (true) {
                auto [task, closed] = cqueue.get();
                if (closed) {
                    break;
                }
                task(cache);
            }
        });
    }

    void close() {
        cqueue.close();
        actor_thread.join();
    }

    std::future<void> do_calc() {
        tasktype calctask(calc);
        auto fut = calctask.get_future();
        cqueue.add(std::move(calctask));
        return fut;
    }
};

static void cache_calc_queue(cachetype& map,
    std::chrono::microseconds sleep_time) {
    cond_queue<std::function<void ()>> cqueue;
    
    auto actor_thread = std::thread([&] {
        std::queue<std::future<void>> futures;
        while (true) {
            auto [val, closed] = cqueue.get();
            if (closed) {
                break;
            }
            val();
        }
    });
    std::vector<std::promise<void>> promises(max_iter);
    for (int i=0; i < max_iter; i++) {
        auto& prom=promises[i];
        cqueue.add([&map, &cqueue, &prom, sleep_time] {
            calc(map);
            std::thread([&map, &prom, &cqueue, sleep_time] {
                fake_io(sleep_time);
                cqueue.add([&map, &prom] {
                    calc(map);
                    prom.set_value();
                });
            }).detach();
        });
    }
    for(auto& p : promises) {
        p.get_future().wait();
    }
    cqueue.close();
    actor_thread.join();


}

static void BM_cachecalc_queue(benchmark::State& state) {
    for (auto _ : state) {
        cachetype cache;
        cache_calc_queue(cache,  std::chrono::microseconds(state.range(0)));
        checkWork(state, cache);
    }
}

BENCHMARK(BM_cachecalc_queue)->Unit(benchmark::kMillisecond)->Range(8, 64);

static void cache_calc_queue1(cachetype& cache,
    std::chrono::microseconds sleep_time) {
    using tasktype = std::packaged_task<void(cachetype&)>;
    cond_queue<tasktype> cqueue;
    
    auto actor_thread = std::thread([&cqueue, &cache] {
        while (true) {
            auto [task, closed] = cqueue.get();
            if (closed) {
                break;
            }
            task(cache);
        }
    });

    std::vector<std::future<void>> futures;
    for (int i=0; i < max_iter; i++) {
        futures.push_back(std::async(std::launch::async,
                [&cqueue, sleep_time] {
            {
                tasktype calctask1(calc);
                auto future1 = calctask1.get_future();
                cqueue.add(std::move(calctask1));
                future1.wait();
            }
            fake_io(sleep_time);
            {
                tasktype calctask2(calc);
                auto future2 = calctask2.get_future();
                cqueue.add(std::move(calctask2));
                future2.wait();
            }
        }));
    }
    for(auto& f : futures) {
        f.wait();
    }
    cqueue.close();
    actor_thread.join();
}


static void BM_cachecalc_queue1(benchmark::State& state) {
    for (auto _ : state) {
        cachetype cache;
        cache_calc_queue1(cache,  std::chrono::microseconds(state.range(0)));
        checkWork(state, cache);
    }
}

BENCHMARK(BM_cachecalc_queue1)->Unit(benchmark::kMillisecond)->Range(8, 64);


static void BM_cachecalc_threadpool(benchmark::State& state) {
    boost::asio::thread_pool pool(num_tasks);
    boost::asio::thread_pool calcpool(1);
    auto strand_ = make_strand(calcpool.get_executor());
    std::chrono::microseconds sleep_time = std::chrono::microseconds(state.range(0));
    for (auto _ : state) {
        cachetype cache;
        std::vector<boost::future<void> > futures;
        futures.reserve(max_iter);
        auto fn = [&] {
            auto f = boost::asio::post(strand_, std::packaged_task<void()>([&] {
                calc(cache);
            }));
            f.wait();
            fake_io(sleep_time);
            auto f1 = boost::asio::post(strand_, std::packaged_task<void()>([&] {
                calc(cache);
            }));
            f1.wait();
        };
        for (int i=0; i < max_iter; i++) {
            boost::packaged_task<void()> task(fn);
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

BENCHMARK(BM_cachecalc_threadpool)->Unit(benchmark::kMillisecond)
->Range(8, 64);




BENCHMARK_MAIN();

