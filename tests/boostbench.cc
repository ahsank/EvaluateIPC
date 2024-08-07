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
#include <boost/thread/future.hpp>
#include <boost/bind/bind.hpp>
#include <boost/asio/deadline_timer.hpp>
#include "benchcommon.hpp"

template <class T, int qsize=100>
class cond_queue {
public:
    std::queue<T> buff;
    volatile bool closed = false;
    std::mutex m_mutex;
    std::condition_variable cvnotempty;
    std::condition_variable cvnotfull;

    cond_queue() {
    }
    void add(T val) {
        {
            std::unique_lock<std::mutex> the_lock(m_mutex);
            cvnotfull.wait(the_lock, [this] {
                return buff.size() < qsize;
            });
            buff.push(std::move(val));
        }
        cvnotempty.notify_one();
    }

    void close() {
        {
            std::unique_lock<std::mutex> the_lock(m_mutex);
            closed = true;
        }
        cvnotempty.notify_all();
    }
    
    std::pair<T, bool> get() {
        T val;
        bool is_closed = false;
        bool popped = false;
        {
            std::unique_lock<std::mutex> the_lock(m_mutex);
            cvnotempty.wait(the_lock, [this] {
                return !buff.empty() || closed;
            });

            if (!buff.empty()) {
                val = std::move(buff.front());
                buff.pop();
                popped = true;
            } else {
                is_closed = closed;  
            }
        }
        if (popped) {
            cvnotfull.notify_one();
        }
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
        actor_thread = std::thread([&] {
            while (true) {
                auto [task, closed] = cqueue.get();
                if (closed) {
                    break;
                }
                task(cache);
            }
        });
    }

    void stop() {
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

static void cache_calc_queue(CacheActor& actor,
    std::chrono::microseconds sleep_time) {
    actor.start();
    std::vector<std::future<void>> futures;
    for (int i=0; i < max_iter; i++) {
        futures.push_back(std::async(std::launch::async,
                [&actor, sleep_time] {
                    actor.do_calc().wait();
                    iotype::fake_io(sleep_time);
                    actor.do_calc().wait();
                }));
    }
    for(auto& f : futures) {
        f.wait();
    }
    actor.stop();
}


static void BM_cachecalc_queue(benchmark::State& state) {
    for (auto _ : state) {
        CacheActor actor;
        cache_calc_queue(actor, std::chrono::microseconds(state.range(0)));
        checkWork(state, actor.cache);
    }
}

BENCHMARK(BM_cachecalc_queue)->Unit(benchmark::kMillisecond)->Range(8, 64);

// Implementatio of the above queue using semaphore
// Not mentioned in benchmark as it had worse performance. Also some decision
// like closing the queue needs more thought
template <class T, int qsize=100>
class sem_queue {
public:
    std::queue<T> buff;
    volatile bool closed = false;
    std::mutex m_mutex;
    std::counting_semaphore<qsize> has_space{qsize};
    std::counting_semaphore<qsize> has_elem{0};

    sem_queue() {
    }
    
    void add(T val) {
        has_space.acquire();
        {
            std::scoped_lock<std::mutex> the_lock(m_mutex);
            buff.push(std::move(val));
        }
        has_elem.release();
    }

    void close() {
        has_space.acquire();
        {
            std::scoped_lock<std::mutex> the_lock(m_mutex);
            closed = true;
        }
        // Behave like adding an element to wake up consumer thread
        has_elem.release();
    }
    
    std::pair<T, bool> get() {
        T val;
        bool is_closed = false;
        bool is_removed = false;
        has_elem.acquire();
        {
            std::scoped_lock<std::mutex> the_lock(m_mutex);
            
            if (closed) {
                is_closed = closed;
            } else {
                val = std::move(buff.front());
                buff.pop();
                is_removed = true;
            }
        }
        if (is_removed) has_space.release();
        return std::make_pair(std::move(val), is_closed);
    }
};

class SemCacheActor {
public:
    cachetype cache;
    using tasktype = std::packaged_task<void(cachetype&)>;
    sem_queue<tasktype> cqueue;
    std::thread actor_thread;

    void start() {
        actor_thread = std::thread([&] {
            while (true) {
                auto [task, closed] = cqueue.get();
                if (closed) {
                    break;
                }
                task(cache);
            }
        });
    }

    void stop() {
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

// Implementation of our task sequence using semaphore
void cache_calc_semqueue(SemCacheActor& actor,
    std::chrono::microseconds sleep_time) {
    actor.start();
    std::vector<std::future<void>> futures;
    for (int i=0; i < max_iter; i++) {
        futures.push_back(std::async(std::launch::async,
                [&actor, sleep_time] {
                    actor.do_calc().wait();
                    iotype::fake_io(sleep_time);
                    actor.do_calc().wait();
                }));
    }
    for(auto& f : futures) {
        f.wait();
    }
    actor.stop();
}


static void BM_cachecalc_semqueue(benchmark::State& state) {
    for (auto _ : state) {
        SemCacheActor actor;
        cache_calc_semqueue(actor, std::chrono::microseconds(state.range(0)));
        checkWork(state, actor.cache);
    }
}

BENCHMARK(BM_cachecalc_semqueue)->Unit(benchmark::kMillisecond)->Range(8, 64);

// In this approach we send a function that does computation and
// asynchronously executes simulated IO operation.
// static void cache_calc_approach2(cachetype& map,
//     std::chrono::microseconds sleep_time) {
//     cond_queue<std::function<void ()>> cqueue;
    
//     auto actor_thread = std::thread([&] {
//         std::queue<std::future<void>> futures;
//         while (true) {
//             auto [val, closed] = cqueue.get();
//             if (closed) {
//                 break;
//             }
//             val();
//         }
//     });
//     std::vector<std::promise<void>> promises(max_iter);
//     for (int i=0; i < max_iter; i++) {
//         auto& prom=promises[i];
//         cqueue.add([&map, &cqueue, &prom, sleep_time] {
//             calc(map);
//             std::thread([&map, &prom, &cqueue, sleep_time] {
//                 iotype::fake_io(sleep_time);
//                 cqueue.add([&map, &prom] {
//                     calc(map);
//                     prom.set_value();
//                 });
//             }).detach();
//         });
//     }
//     for(auto& p : promises) {
//         p.get_future().wait();
//     }
//     cqueue.close();
//     actor_thread.join();
// }

// static void BM_cachecalc_approach2(benchmark::State& state) {
//     for (auto _ : state) {
//         cachetype cache;
//         cache_calc_approach2(cache,  std::chrono::microseconds(state.range(0)));
//         checkWork(state, cache);
//     }
// }

// BENCHMARK(BM_cachecalc_approach2)->Unit(benchmark::kMillisecond)->Range(0, 64);


void BM_cachecalc_boost_threadpool(benchmark::State& state) {
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
            iotype::fake_io(sleep_time);
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
        boost::wait_for_all(futures.begin(), futures.end());
        checkWork(state, cache);
    }
}

BENCHMARK(BM_cachecalc_boost_threadpool)->Unit(benchmark::kMillisecond)
->Range(8, 64);


BENCHMARK_MAIN();

