CC := gcc
CFLAGS := -O2 -Wall -Wextra

compile:
	$(CC) $(CFLAGS) ./src/*.c ./src/**/*.c -o ./telly

clean:
	rm -f ./telly
