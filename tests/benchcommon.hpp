#pragma once
#include <string>
#include <unordered_map>
#include <chrono>
#include <fstream>
#include <thread>
#include <benchmark/benchmark.h>


using namespace std::literals::chrono_literals;
const unsigned int max_iter = 1000;
const unsigned int num_calc = 10;
constexpr unsigned int num_tasks = 8;
constexpr int start_val = 10000;

using cachetype = std::unordered_map<std::string, std::string>;
const std::string cache_key = "hello";
struct sleep_io {
    static inline void fake_io (std::chrono::microseconds sleep_time) {
        std::this_thread::sleep_for(sleep_time);
    }
};

inline std::string get_key(int i) {return cache_key + std::to_string(i);}

inline void calc(cachetype& cache) {
    // Some complex calculations
    for (int i=0; i < num_calc; i++) {
        auto key = get_key(i);
        auto str = cache[key];
        // converts to int, increments and converts back to string
        cache[key] = str.length() == 0 ?
            std::to_string(start_val+1) : std::to_string(std::stoi(str) + 1);
    }
}

template <typename T> void log_error(benchmark::State& state, T result, T check) {
    std::stringstream os;
    os << "Result difference " << result << ", " << check << std::endl;
    state.SkipWithError(os.str());
 
}

// Check that there was no race condtion. The final value in the hash table
// should be start_val + iterations * 2
inline void checkWork(benchmark::State& state, const cachetype& cache,
    unsigned iter = max_iter) {
    auto check_val = start_val + iter * 2;
    const auto str_check =  std::to_string(check_val);
    for (int i=0; i < num_calc; i++) {
        auto str = cache.at(get_key(i));
        if (str != str_check) {
            log_error(state, str, str_check);
        }
    }
}

// More accurate sleep
struct devzero_io {
    static inline void fake_io(std::chrono::microseconds sleep_time) {
        if (sleep_time <= 0us) return;
        using clock =  std::chrono::steady_clock;
        auto end_time = clock::now() + sleep_time;
        std::ifstream zfs("/dev/zero");
        const int readsize = 1024;
        char v[readsize];
        while ( clock::now() < end_time) {
            zfs.read(v, readsize);
        }
        zfs.close();
    }
};

using iotype = devzero_io;
