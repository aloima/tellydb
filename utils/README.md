## Benchmark Results
Tested on Intel Core i7-7500U x 4 using [benchmark/benchmark.c](./benchmark/benchmark.c)
```
Benchmark results (100000 operations per server):
telly master test: SET=5341.05 ms, GET=5470.35 ms, PING=4767.85 ms
valkey 8.0.2 test: SET=4651.63 ms, GET=4574.68 ms, PING=4570.31 ms
```
