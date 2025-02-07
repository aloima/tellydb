CC := gcc
LIBRARIES := -lm -lpthread $(shell pkg-config --cflags --libs openssl) -lcrypt
CFLAGS := -O3 -Wall -Wextra \
-D_GNU_SOURCE -D_LARGEFILE64_SOURCE \
-DGIT_HASH=\"$(shell git rev-parse HEAD)\" -DVERSION=\"$(shell git describe --abbrev=0 --tags)\"

compile: benchmark/benchmark.o telly

benchmark/benchmark.o: benchmark/benchmark.c
	@$(CC) $< -o $@ -lhiredis
	@echo Benchmark file is compiled.

telly: ./src/*.c ./src/**/*.c ./src/**/**/*.c
	@$(CC) $(CFLAGS) $^ -o $@ $(LIBRARIES)
	@echo Compiled.

clean:
	rm -f ./telly
	rm -rf ./benchmark/benchmark.o
