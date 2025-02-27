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


void client(char* filename) {
    int socket_desc;
    char buffer[2048];
    char sql[1024]; 

    while ((socket_desc = domain_socket_client_create(filename)) < 0) {
        // infinitely wait until user connects
    }
    printf("Client connected\n");
    if (socket_desc < 0) panic("domain socket create");

	while(1){
		// Get SQL query from user input
		printf("Enter SQL query: ");
		if (!fgets(sql, sizeof(sql), stdin)) {
			perror("Error reading input");
			close(socket_desc);
			return;
		}

		// remove trailing line
		size_t len = strlen(sql);
		if (len > 0 && sql[len - 1] == '\n') {
			sql[len - 1] = '\0';
		}

		// Send the query to the server
		if (write(socket_desc, sql, strlen(sql)) == -1) {
			panic("Unable to write");
		}

		// Read server response
		ssize_t num_read = read(socket_desc, buffer, sizeof(buffer) - 1);
		if (num_read > 0) {
			buffer[num_read] = '\0';
			printf("Server response: %s\n", buffer);
		} else {
			perror("Error reading from server");
		}
	}

    close(socket_desc);
}


int main(int argc, char* argv[]) {

  char *ds = "domain_socket";

  client(ds);


  return 0;
}