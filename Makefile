CC      = gcc
CFLAGS  = -Wall -Wextra -std=c99 -MMD -MP
TARGET  = banker

SRCS    = main.c init.c matriz.c banker.c
OBJS    = $(SRCS:.c=.o)
DEPS    = $(OBJS:.o=.d)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

-include $(DEPS)

clean:
	rm -f *.o *.d banker banker.exe

.PHONY: all clean