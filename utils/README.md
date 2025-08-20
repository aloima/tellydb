## Benchmark Results
Tested on Intel Core i7-7500U x 4 using [benchmark/benchmark.c](./benchmark/benchmark.c)
```
Benchmark results (100000 operations per server):
telly master test: SET=5306.71 ms, GET=5016.56 ms, PING=4299.55 ms
valkey 8.0.4 test: SET=4600.59 ms, GET=4375.43 ms, PING=4183.49 ms
```
