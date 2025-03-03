CC=gcc
INCDIRS=-I.
OPT=-O0
CFLAGS=-Wall -Wextra -g $(INCDIRS) $(OPT)

SERVER_CFILES=scm_cred_recv.c unix_sockets.c error_functions.c
CLIENT_CFILES=scm_cred_send.c unix_sockets.c error_functions.c

SERVER_OBJECTS=scm_cred_recv.o unix_sockets.o error_functions.o
CLIENT_OBJECTS=scm_cred_send.o unix_sockets.o error_functions.o

BINARY=server client

all: $(BINARY)

server: $(SERVER_OBJECTS)
		$(CC) -o $@ $^
		
client: $(CLIENT_OBJECTS)
		$(CC) -o $@ $^

%.o:%.c
		$(CC) $(CFLAGS) -c -o $@ $^

clean:
		rm -rf $(BINARY) *.o