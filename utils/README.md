## Benchmark Results
Tested on Intel Core i7-7500U x 4 using [benchmark/benchmark.c](./benchmark/benchmark.c)
```
Benchmark results (100000 operations per server):
telly master test: SET=5149.97 ms, GET=5061.86 ms, PING=4308.03 ms
valkey 8.0.2 test: SET=4369.79 ms, GET=4226.98 ms, PING=4186.33 ms
```
