include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
FetchContent_Declare(googlebenchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG main)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(
googletest
googlebenchmark
)

enable_testing()


add_library(tests INTERFACE)
include_directories(tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(
  mutexbench
  mutexbench.cc
)

target_link_libraries(
  mutexbench
  benchmark::benchmark
)

if(ENABLE_FOLLY)
  add_executable(
    follybench
    follybench.cc
  )

  target_link_libraries(
    follybench
    Folly::folly
    benchmark::benchmark
  )
endif()

FIND_PACKAGE( Boost  COMPONENTS program_options  thread system REQUIRED )
INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIR} )

add_executable(
  boostbench
  boostbench.cc
)

target_link_libraries(
  boostbench
  benchmark::benchmark
  ${Boost_LIBRARIES}
)


include(GoogleTest)

add_subdirectory(seastar EXCLUDE_FROM_ALL)
