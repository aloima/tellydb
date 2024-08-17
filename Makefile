CC := gcc
CFLAGS := -O2 -Wall -Wextra
LIBRARIES := -lm -lpthread

compile:
	$(CC) $(CFLAGS) ./src/*.c ./src/**/*.c -o ./telly $(LIBRARIES)

clean:
	rm -f ./telly
