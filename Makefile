GIT_HASH ?= $(shell git rev-parse HEAD)
GIT_VERSION ?= $(shell git describe --abbrev=0 --tags)

CC ?= gcc
LIBRARIES ?= -lm -lpthread $(shell pkg-config --cflags --libs openssl) -lcrypt
CFLAGS ?= -O3 -Wall -Wextra -D_GNU_SOURCE -D_LARGEFILE64_SOURCE -DGIT_HASH=\"$(GIT_HASH)\" -DVERSION=\"$(GIT_VERSION)\"

compile: utils/benchmark utils/tests telly

utils/benchmark: utils/benchmark.c
	@$(CC) $< -o $@ -lhiredis
	@echo Benchmark file is compiled.

utils/tests: utils/tests.c
	@$(CC) $< -o $@ -lhiredis -lpthread
	@echo Tests file is compiled.

telly: ./src/*.c ./src/**/*.c ./src/**/**/*.c
	@$(CC) $(CFLAGS) $^ -o $@ $(LIBRARIES)
	@echo Compiled.

clean:
	rm -f ./telly ./utils/benchmark ./utils/tests

dockerize:
	docker build --build-arg GIT_HASH=$(GIT_HASH) --build-arg GIT_VERSION=$(GIT_VERSION) -t tellydb .
