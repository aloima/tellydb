CC := gcc
LIBRARIES := -lm -lpthread $(shell pkg-config --cflags --libs openssl) -lcrypt
CFLAGS := -O3 -Wall -Wextra \
-D_GNU_SOURCE -D_LARGEFILE64_SOURCE \
-DGIT_HASH=\"$(shell git rev-parse HEAD)\" -DVERSION=\"$(shell git describe --abbrev=0 --tags)\"

compile:
	@$(CC) $(CFLAGS) ./src/*.c ./src/**/*.c ./src/**/**/*.c -o ./telly $(LIBRARIES)
	@echo Compiled.

clean:
	rm -f ./telly
