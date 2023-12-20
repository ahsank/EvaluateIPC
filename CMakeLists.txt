# https://matgomes.com/integrate-google-test-into-cmake/
cmake_minimum_required(VERSION 3.5)
set(default_build_type "Release")

# set(CMAKE_C_COMPILER "/usr/bin/gcc")
# set(CMAKE_CXX_COMPILER "/usr/bin/g++")

project(MultiplyTest LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# add_definitions(-DPARALLEL)

# set(CMAKE_CXX_FLAGS "-std=c++23 -stdlib=libc++ -fcoroutines-ts")
set(CMAKE_CXX_FLAGS "-std=c++23 -fcoroutines")
enable_testing()

# add_subdirectory(src)
add_subdirectory(tests)
