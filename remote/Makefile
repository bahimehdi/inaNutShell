# Makefile — builds the remote shell's two programs: 'server' and 'client'.

CC = gcc  # the C compiler to use
CFLAGS = -Wall -Wextra -Werror -g  # -Wall/-Wextra: enable warnings; -Werror: warnings stop the build; -g: include debug info

all: server client  # the default target — running plain "make" builds both programs

server: server.c  # the 'server' executable is rebuilt whenever server.c changes
	$(CC) $(CFLAGS) -o server server.c  # compile server.c into the executable named 'server'

client: client.c  # the 'client' executable is rebuilt whenever client.c changes
	$(CC) $(CFLAGS) -pthread -o client client.c  # compile client.c; -pthread links the POSIX threads library

clean:  # "make clean" deletes everything the build produced
	rm -f server client *.o  # remove the two executables and any leftover object files

.PHONY: all clean  # tell make that 'all' and 'clean' are command names, not real files
