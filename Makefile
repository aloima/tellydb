CC := gcc
LIBRARIES := -lm -lpthread$(shell pkg-config --cflags --libs openssl)
CFLAGS := -O3 -Wall -Wextra \
-D_GNU_SOURCE \
-DGIT_HASH=\"$(shell git rev-parse HEAD)\" -DVERSION=\"$(shell git describe --abbrev=0 --tags)\"

compile:
	$(CC) $(CFLAGS) ./src/*.c ./src/**/*.c ./src/**/**/*.c -o ./telly $(LIBRARIES)

clean:
	rm -f ./telly
