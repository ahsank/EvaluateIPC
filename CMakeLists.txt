# https://matgomes.com/integrate-google-test-into-cmake/
cmake_minimum_required(VERSION 3.5)
if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
cmake_policy(SET CMP0135 NEW)
endif()

# Set default build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build (Debug, Release, RelWithDebInfo, MinSizeRel)" FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
  message(STATUS "Setting build type to '${CMAKE_BUILD_TYPE}' as none was specified.")
endif()

# set(CMAKE_C_COMPILER "/usr/bin/gcc")
# set(CMAKE_CXX_COMPILER "/usr/bin/g++")

project(MultiplyTest LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# add_definitions(-DPARALLEL)

# set(CMAKE_CXX_FLAGS "-std=c++23 -stdlib=libc++ -fcoroutines-ts")
set(CMAKE_CXX_FLAGS "-std=c++20")
enable_testing()

# Optional components
option(ENABLE_FOLLY "Build with Folly support" OFF)

if(ENABLE_FOLLY)
  add_subdirectory(folly)
endif()

add_subdirectory(src)
add_subdirectory(tests)
