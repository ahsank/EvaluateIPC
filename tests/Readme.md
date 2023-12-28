For bulid instructions see the root folder.


## Benchmark results 

Macbook pro Apple M2, 8 cores, 16 GB RAM with sleeptime 8 us and 64 us https://ahsank.github.io/posts/MutexBasedSynchronization/
  

```console
root@ccb37d059b3e:/build# ./tests/mutexbench 
2023-12-21T20:21:58+00:00
Running ./tests/mutexbench
Run on (4 X 48 MHz CPU s)
Load Average: 0.40, 0.24, 0.14
```

----------------------------------------------------------------
Benchmark              |        Time   |          CPU  | Iterations
-----------------------| ------------- | ------------  | -----------
BM_cachecalc/8         |     85.0 ms   |     0.044 ms  |        100
BM_cachecalc/64        |      155 ms   |     0.044 ms  |        100
BM_cachecalc_mutex/8   |     84.8 ms   |     0.048 ms  |        100
BM_cachecalc_mutex/64  |      155 ms   |     0.042 ms  |        100
BM_cachecalc_async/8   |     17.5 ms   |      13.6 ms  |         48
BM_cachecalc_async/64  |     24.0 ms   |      15.4 ms  |         44

