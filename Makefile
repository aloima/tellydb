GIT_HASH ?= $(shell git rev-parse HEAD)
GIT_VERSION ?= $(shell git describe --abbrev=0 --tags)

FEATURE_FLAGS ?= -D_TIME_BITS=64 -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64

CC ?= gcc
LIBRARIES ?= -lm -lpthread $(shell pkg-config --cflags --libs openssl)
CFLAGS ?= -O3 -Wall -Wextra $(FEATURE_FLAGS) -DGIT_HASH=\"$(GIT_HASH)\" -DVERSION=\"$(GIT_VERSION)\"

compile: utils/benchmark utils/tests telly

utils/benchmark: utils/benchmark.c
	@$(CC) $< -o $@ $(shell pkg-config --cflags --libs hiredis)
	@echo Benchmark file is compiled.

utils/tests: utils/tests.c
	@$(CC) $< -o $@ $(shell pkg-config --cflags --libs hiredis) -lpthread
	@echo Tests file is compiled.

telly: ./src/*.c ./src/**/*.c ./src/**/**/*.c
	@$(CC) $(CFLAGS) $^ -o $@ $(LIBRARIES)
	@echo Compiled.

clean:
	rm -f ./telly ./utils/benchmark ./utils/tests

dockerize:
	docker build --build-arg GIT_HASH=$(GIT_HASH) --build-arg GIT_VERSION=$(GIT_VERSION) -t tellydb .
