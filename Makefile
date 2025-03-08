CC=gcc
CC=gcc
INCDIRS=-I.
OPT=-O0
CFLAGS=-Wall -Wextra -g $(INCDIRS) $(OPT)

SERVER_CFILES=server.c scm_cred_recv.c unix_sockets.c error_functions.c
CLIENT_CFILES=client.c scm_cred_send.c unix_sockets.c error_functions.c

SERVER_OBJECTS=server.o scm_cred_recv.o unix_sockets.o error_functions.o
CLIENT_OBJECTS=client.o scm_cred_send.o unix_sockets.o error_functions.o

TEST_CFILES=test_client.c scm_cred_send.c scm_cred_recv.c unix_sockets.c error_functions.c
TEST_OBJECTS=test_client.o scm_cred_recv.o scm_cred_send.o unix_sockets.o error_functions.o


BINARY=server client client_test

all: $(BINARY)

server: $(SERVER_OBJECTS)
		$(CC) -o $@ $^ -lsqlite3 -ljson-c
		
client: $(CLIENT_OBJECTS)
		$(CC) -o $@ $^ -lsqlite3 -ljson-c
	
client_test: $(TEST_OBJECTS)
		$(CC) -o $@ $^ -lsqlite3 -ljson-c

%.o:%.c
		$(CC) $(CFLAGS) -c -o $@ $^

clean:
		rm -rf $(BINARY) *.o
