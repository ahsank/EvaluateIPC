For bulid instructions see the root folder.

It implements the benchmarks mentioned in blogs  [Mutex Based Synchronization](https://ahsank.github.io/posts/MutexBasedSynchronization) and [Actor Framework](https://ahsank.github.io/posts/ActorBasedSynchronization/)

## Benchmark results 

Macbook pro Apple M2, 8 cores, 16 GB RAM:
  
### Mutex based synchronization

```console
root@ccb37d059b3e:/build# ./tests/mutexbench 
2023-12-21T20:21:58+00:00
Running ./tests/mutexbench
Run on (4 X 48 MHz CPU s)
Load Average: 0.40, 0.24, 0.14
```

---------------------------------------------------------------
Benchmark              |        Time   |          CPU  | Iterations
---------------------- | ------------- | --------------| ---------
BM_cachecalc/8         |     85.0 ms   |     0.044 ms  |        100
BM_cachecalc/64        |      155 ms   |     0.044 ms  |        100
BM_cachecalc_mutex/8   |     84.8 ms   |     0.048 ms  |        100
BM_cachecalc_mutex/64  |      155 ms   |     0.042 ms  |        100
BM_cachecalc_async/8   |     17.5 ms   |      13.6 ms  |         48
BM_cachecalc_async/64  |     24.0 ms   |      15.4 ms  |         44


### Actor framework

```console
root@ccb37d059b3e:/build# ./tests/boostbench 
2023-12-28T20:56:50+00:00
Running ./tests/boostbench
Run on (4 X 48 MHz CPU s)
Load Average: 0.26, 0.06, 0.02
```

---------------------------------------------------------------------
Benchmark                 |        Time     |       CPU  | Iterations
--------------------------|-----------------| -----------| -----------
BM_cachecalc_queue/8      |      22.9 ms    |    21.5 ms |         31
BM_cachecalc_queue/64     |      23.6 ms    |    21.8 ms |         32
BM_cachecalc_approach2/8  |      27.0 ms    |    5.40 ms |        124
BM_cachecalc_approach2/64 |      26.7 ms    |    5.14 ms |        136
BM_cachecalc_threadpool/8 |      16.5 ms    |    2.40 ms |        293
BM_cachecalc_threadpool/64 |     25.6 ms    |    2.47 ms |        100
```
