CC = gcc
CFLAGS = -Wall -Wextra -Werror -g
SRCS = main.c parser.c executor.c builtins.c signals.c utils.c
OBJS = $(SRCS:.c=.o)
TARGET = inaNutShell

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
