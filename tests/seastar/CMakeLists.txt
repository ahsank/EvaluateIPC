
find_package (Seastar QUIET)

# TODO: Following to download and build seastar doesn't work, fix later
# include(ExternalProject)
# IF (NOT Seastar_FOUND)
#
# ExternalProject_Add(Seastar
#  GIT_REPOSITORY https://github.com/scylladb/seastar.git
#  GIT_TAG seastar-22.11.0
#  UPDATE_COMMAND      ""
#  CONFIGURE_COMMAND ./configure.py --mode=release
#  BUILD_COMMAND ninja -C build/release
#  INSTALL_COMMAND ninja -C build/release install
# )
# add_library(Seastar::seastar STATIC IMPORTED GLOBAL)
#
# ENDIF()

set(CMAKE_CXX_STANDARD 23)
enable_testing()

IF ( Seastar_FOUND )

add_executable(
  seastar_bench
  seastar_bench.cc
 )

target_link_libraries(
  seastar_bench
  PRIVATE Seastar::seastar
  benchmark::benchmark
  tests
)

add_executable(
  seastar_test
  seastar_test.cc
)


target_link_libraries(
  seastar_test
  PRIVATE Seastar::seastar
  gtest_main
  gmock_main
)


include(GoogleTest)

gtest_discover_tests(seastar_test)

ENDIF()

