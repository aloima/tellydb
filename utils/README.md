## Benchmark Results
Tested on Intel Core i7-7500U x 4 using [benchmark/benchmark.c](./benchmark/benchmark.c)
```
Benchmark results (100000 operations per server):
telly master test: SET=5061.81 ms, GET=4899.41 ms, PING=4150.80 ms
valkey 8.0.2 test: SET=4472.54 ms, GET=4238.61 ms, PING=4058.16 ms
```
