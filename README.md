To run benchmarks

In your desktop
===============

Go to root folder

```console
cmake -B build -DENABLE_FOLLY=ON
cmake --build build
```

Using docker
============

From root of this repo

```console
./bin/rundocker --build
./bin/rundocker
```

Inside the docker container

```console
/build# cmake -DCMAKE_BUILD_TYPE=Release /workspace
```

Or

```console
/build# /workspace/bin/runcmakeindocker.sh
```

Then

```console
/build# make
/build# ./tests/mutexbench
/build# ./tests/boostbench
```

etc

From Local Ubuntu machine
================

Run

```console
./docker/cmake/install-dependencies.sh
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
./tests/mutexbench
```

Folly benchmark
===============

```console
sudo apt install libfast-float-dev
cmake -B build -DENABLE_FOLLY=ON /workspace
cmake --build build
/build# ./tests/follybench 
2024-07-24T21:20:05+00:00
Running ./tests/follybench
Run on (4 X 48 MHz CPU s)
Load Average: 0.29, 0.12, 0.24
---------------------------------------------------------------------------
Benchmark                                 Time             CPU   Iterations
---------------------------------------------------------------------------
BM_cachecalc_folly_threadpool/0       0.091 ms        0.023 ms        30034
BM_cachecalc_folly_threadpool/1       0.089 ms        0.023 ms        28378
BM_cachecalc_folly_threadpool/8       0.088 ms        0.023 ms        29926
BM_cachecalc_folly_threadpool/64      0.219 ms        0.024 ms        29769
done
```

Seems great comparing it to `seastar` benchmark


```console
/build$ ./tests/seastar/seastar_bench 
WARNING: unable to mbind shard memory; performance may suffer: Operation not permitted
INFO  2024-07-24 21:27:33,514 seastar - Reactor backend: io_uring
WARN  2024-07-24 21:27:33,520 seastar - Creation of perf_event based stall detector failed: falling back to posix timer: std::system_error (error system:1, perf_event_open() failed: Operation not permitted)
WARNING: unable to mbind shard memory; performance may suffer: Operation not permitted
WARN  2024-07-24 21:27:33,571 seastar - Creation of perf_event based stall detector failed: falling back to posix timer: std::system_error (error system:1, perf_event_open() failed: Operation not permitted)
WARNING: unable to mbind shard memory; performance may suffer: Operation not permitted
WARN  2024-07-24 21:27:33,575 seastar - Creation of perf_event based stall detector failed: falling back to posix timer: std::system_error (error system:1, perf_event_open() failed: Operation not permitted)
WARNING: unable to mbind shard memory; performance may suffer: Operation not permitted
WARN  2024-07-24 21:27:33,582 seastar - Creation of perf_event based stall detector failed: falling back to posix timer: std::system_error (error system:1, perf_event_open() failed: Operation not permitted)
Running benchmarks...
2024-07-24T21:27:33+00:00
Running ./tests/seastar/seastar_bench
Run on (4 X 48 MHz CPU s)
Load Average: 0.00, 0.03, 0.15
------------------------------------------------------------
Benchmark                  Time             CPU   Iterations
------------------------------------------------------------
BM_SeastarHash/0        1.17 ms         1.17 ms          609
BM_SeastarHash/1        1.16 ms         1.16 ms          605
BM_SeastarHash/8        1.18 ms         1.18 ms          604
BM_SeastarHash/64       1.23 ms         1.23 ms          569
done
```

## Debugging
Before debugging in gdb run following commands to avoid verbose log

```
set print thread-events off
```