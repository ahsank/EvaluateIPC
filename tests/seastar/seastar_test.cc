#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <chrono>
#include <ratio>
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

// Demonstrate some basic assertions.
TEST(SeastarTest, BasicAssertions) {
  // Expect two strings not to be equal.
  EXPECT_STRNE("hello", "world");
  // Expect equality.
  EXPECT_EQ(7 * 6, 42);
}

using namespace std::chrono_literals;
TEST(SeastarTest, SleepTest) {
    auto start = std::chrono::high_resolution_clock::now();
    seastar::smp::submit_to(1, [] {
        return seastar::sleep(8us);
    }).then([start]{
        auto real_duration = duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now() - start);
        EXPECT_GT(real_duration, 8us);
    }).get();
    
}

TEST(SeastarTest, BaseSleepTest) {
    auto start =  std::chrono::high_resolution_clock::now();
    std::this_thread::sleep_for(8us);
    auto real_duration = duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now() - start);
    EXPECT_GT(real_duration, 8us);
}

int main(int argc, char **argv) {
    seastar::app_template app;
    app.run(argc, argv, [argc, argv] () {
        return seastar::async([&] {
            int ac = argc;
            std::cout << "Running benchmarks...\n";
            ::testing::InitGoogleTest(&ac, argv);
            return RUN_ALL_TESTS();
            std::cout << "done\n";
        });
    });
}
