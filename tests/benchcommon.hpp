#pragma once
#include <string>
#include <unordered_map>
#include <chrono>

using namespace std::literals::chrono_literals;
const unsigned int max_iter = 1000;
const unsigned int num_calc = 10;
constexpr unsigned int num_tasks = 8;

using cachetype = std::unordered_map<std::string, std::string>;
const std::string cache_key = "hello";
inline void fake_io (std::chrono::microseconds sleep_time=8us) {
    std::this_thread::sleep_for(sleep_time);
}

inline std::string get_key(int i) {return cache_key + std::to_string(i);}

inline void calc(cachetype& cache) {
    // Some complex calculations
    for (int i=0; i < num_calc; i++) {
        auto key = get_key(i);
        auto str = cache[key];
        cache[key] = str.length() == 0 ?
            "1" : std::to_string(std::stoi(str) + 1);
    }
}

template <typename T> void log_error(benchmark::State& state, T result, T check) {
    std::stringstream os;
    os << "Result difference " << result << ", " << check << std::endl;
    state.SkipWithError(os.str());
 
}
inline void checkWork(benchmark::State& state, const cachetype& cache,
    unsigned check_val = max_iter*2) {
    const auto str_check =  std::to_string(check_val);
    for (int i=0; i < num_calc; i++) {
        auto str = cache.at(get_key(i));
        if (str != str_check) {
            log_error(state, str, str_check);
        }
    }
}
