## Benchmark Results
Tested on Intel Core i7-7500U x 4 using [benchmark/benchmark.c](./benchmark/benchmark.c)
```
Benchmark results (100000 operations per server):
telly master test: SET=5334.93 ms, GET=4995.98 ms, PING=4455.65 ms
valkey 8.0.4 test: SET=4765.66 ms, GET=4545.88 ms, PING=4327.41 ms
```
