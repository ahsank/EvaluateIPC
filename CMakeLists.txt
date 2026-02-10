# https://matgomes.com/integrate-google-test-into-cmake/
cmake_minimum_required(VERSION 3.5)
if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
cmake_policy(SET CMP0135 NEW)
endif()

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

# Optional components
option(ENABLE_FOLLY "Build with Folly support" OFF)

if(ENABLE_FOLLY)
  add_subdirectory(folly)
endif()

add_subdirectory(src)
add_subdirectory(tests)
