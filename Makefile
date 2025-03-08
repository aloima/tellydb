CC := gcc
LIBRARIES := -lm -lpthread $(shell pkg-config --cflags --libs openssl) -lcrypt
CFLAGS := -O3 -Wall -Wextra \
-D_GNU_SOURCE -D_LARGEFILE64_SOURCE \
-DGIT_HASH=\"$(shell git rev-parse HEAD)\" -DVERSION=\"$(shell git describe --abbrev=0 --tags)\"

compile: utils/benchmark utils/tests telly

utils/benchmark: utils/benchmark.c
	@$(CC) $< -o $@ -lhiredis
	@echo Benchmark file is compiled.

utils/tests: utils/tests.c
	@$(CC) $< -o $@ -lhiredis
	@echo Tests file is compiled.

telly: ./src/*.c ./src/**/*.c ./src/**/**/*.c
	@$(CC) $(CFLAGS) $^ -o $@ $(LIBRARIES)
	@echo Compiled.

clean:
	rm -f ./telly ./utils/benchmark ./utils/tests
