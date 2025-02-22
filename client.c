#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <assert.h>
#include <sqlite3.h>
#include <sys/poll.h>
#include "server_ds.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 


void client(char* filename){
	int i, socket_desc;
	char* sql; 
	char buffer[2048];

    while ((socket_desc = domain_socket_client_create(filename)) < 0) {
		// perror("Waiting for server to be ready...");

	}
	printf("client connected");
    if (socket_desc < 0) panic("domain socket create");


	sql = "SELECT * from user;";

	if(write(socket_desc, sql, strlen(sql)) == -1) panic("unable to write");

	ssize_t num_read = read(socket_desc, buffer, 2048);
    if (num_read > 0) {
        buffer[num_read] = '\0';
        printf("Server response: %s\n", buffer);
    }
	

	close(socket_desc);
	return;
}


int main(int argc, char* argv[]) {

  char *ds = "domain_socket";

  client(ds);


  return 0;
}