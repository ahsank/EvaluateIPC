Bechmark measuring a multhreaded operation using [seastar](https://seastar.io/) framework.

### To build

Build and run the docker container using the instruction in the repo [README.md](/README.md).
Then install `seastar` by

```console
# cd /
# git clone --depth 1  https://github.com/scylladb/seastar.git
# cd /seastar/
# ./install-dependencies.sh
# ./configure.py --mode=release
# ninja -C build/release -j 2
# ninja -C build/release -j 2 install
```

Then build and run the benchmark

```console
/# cd /build/
/build# cmake /workspace/
/build# make seastar_bench
/build# ./tests/seastar/seastar_bench 
WARNING: unable to mbind shard memory; performance may suffer: 
INFO  2024-01-02 23:01:11,735 seastar - Reactor backend: linux-aio
WARN  2024-01-02 23:01:11,753 [shard 0] seastar - Creation of perf_event based stall detector failed, falling back to posix timer: std::system_error (error system:1, perf_event_open() failed: Operation not permitted)
INFO  2024-01-02 23:01:11,777 [shard 0] seastar - Created fair group io-queue-0, capacity rate 2147483:2147483, limit 12582912, rate 16777216 (factor 1), threshold 2000
INFO  2024-01-02 23:01:11,777 [shard 0] seastar - IO queue uses 0.75ms latency goal for device 0
INFO  2024-01-02 23:01:11,777 [shard 0] seastar - Created io group dev(0), length limit 4194304:4194304, rate 2147483647:2147483647
INFO  2024-01-02 23:01:11,777 [shard 0] seastar - Created io queue dev(0) capacities: 512:2000:2000 1024:3000:3000 2048:5000:5000 4096:9000:9000 8192:17000:17000 16384:33000:33000 32768:65000:65000 65536:129000:129000 131072:257000:257000
WARNING: unable to mbind shard memory; performance may suffer: 
WARN  2024-01-02 23:01:11,778 [shard 1] seastar - Creation of perf_event based stall detector failed, falling back to posix timer: std::system_error (error system:1, perf_event_open() failed: Operation not permitted)
WARNING: unable to mbind shard memory; performance may suffer: 
WARN  2024-01-02 23:01:11,779 [shard 3] seastar - Creation of perf_event based stall detector failed, falling back to posix timer: std::system_error (error system:1, perf_event_open() failed: Operation not permitted)
WARNING: unable to mbind shard memory; performance may suffer: 
WARN  2024-01-02 23:01:11,783 [shard 2] seastar - Creation of perf_event based stall detector failed, falling back to posix timer: std::system_error (error system:1, perf_event_open() failed: Operation not permitted)
Running benchmarks...
2024-01-02T23:01:11+00:00
Running ./tests/seastar/seastar_bench
Run on (4 X 48 MHz CPU s)
Load Average: 0.84, 0.40, 0.21
------------------------------------------------------------
Benchmark                  Time             CPU   Iterations
------------------------------------------------------------
BM_SeastarHash/8        1.29 ms         1.20 ms          570
BM_SeastarHash/64       1.25 ms         1.25 ms          550
done

```
