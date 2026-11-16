CC     = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2

banker: main.c
	$(CC) $(CFLAGS) -o banker main.c

clean:
	rm -f banker result.txt