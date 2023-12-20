#pragma once
#include <string>
#include <unordered_map>
#include <chrono>

using namespace std::literals::chrono_literals;
const unsigned int max_iter = 1000;
constexpr unsigned int num_tasks = 8;

using cachetype = std::unordered_map<std::string, std::string>;
const std::string str_check = std::to_string(max_iter);
const auto sleep_time = 10us;
const std::string cache_key = "hello";
inline void fake_io () {
    std::this_thread::sleep_for(sleep_time);
}

inline void calc(cachetype& cache) {
    auto str = cache[cache_key];
    cache[cache_key] = str.length() == 0 ? "1" : std::to_string(std::stoi(str) + 1);
}

template <typename T> void log_error(benchmark::State& state, T result, T check) {
    std::stringstream os;
    os << "Result difference " << result << ", " << check << std::endl;
    state.SkipWithError(os.str());
 
}
inline void checkWork(benchmark::State& state, const cachetype& cache) {
    auto str = cache.at(cache_key);
    if (str != str_check) {
        log_error(state, str, str_check);
    }
}
