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


#define DB_PATH "database/regs.db"
#define MAX_FDS 16




static int cb_send_results(void *socket_fd, int argc, char **argv, char **azColName) {
    int client_socket = *(int *)socket_fd;  // Cast void* back to int*
    char buffer[1024];  // Temporary buffer to hold each row's data

    // Construct the message for each row
    buffer[0] = '\0';  // Reset buffer
    for (int i = 0; i < argc; i++) {
        snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), 
                 "%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    strcat(buffer, "\n");  // Add a newline after each row

    // Send the data to the client
    if (send(client_socket, buffer, strlen(buffer), 0) < 0) {
        perror("Send failed");
        return 1;  // Stop the callback on error
    }

    return 0;  // Continue processing
}


// -1 or error
sqlite3* initate_db(){
    sqlite3 *db;
    int rc;

    rc = sqlite3_open(DB_PATH, &db);
    return db;

}


// poll to get query
// call sqlite3 on query on server
// return output to specfic client

void server(char *filename){
    sqlite3* db  = initate_db();
	if(db){
		printf("successful got database.\n");
	}
	struct pollfd poll_fds[MAX_FDS];
	// create the socket
	int socket_desc, num_fds = 0;
	socket_desc = domain_socket_server_create(filename);

	/* Initialize all pollfd structs to 0 */
	memset(poll_fds, 0, sizeof(struct pollfd) * MAX_FDS);
	poll_fds[0] = (struct pollfd) {
		 .fd     = socket_desc,
		 .events = POLLIN,
	 };
	num_fds++;

	// event loop
	for(;;){
		int ret, new_client,i;
		char *zErrMsg;
		
		ret = poll(poll_fds, num_fds, -1);
		if (ret == -1) panic("poll error");
		
		if (poll_fds[0].revents & POLLIN) {
            if ((new_client = accept(socket_desc, NULL, NULL)) == -1) panic("server accept");
			
			if (num_fds < MAX_FDS) {
				/* add a new file descriptor! */
				poll_fds[num_fds] = (struct pollfd) {
					.fd = new_client,
					.events = POLLIN
				};
				num_fds++;
				poll_fds[0].revents = 0;
				printf("server: created client connection %d\n", new_client);
			}else{
				close(new_client);
			}
		}
		for(i = 1; i < num_fds; i++){
            if (poll_fds[i].revents & (POLLHUP | POLLERR)) {
                printf("server: closing client connection %d\n", poll_fds[i].fd);
                poll_fds[i].revents = 0;
                close(poll_fds[i].fd);
                /* replace the fd to fill the gap */
                poll_fds[i] = poll_fds[num_fds - 1];
                num_fds--;
                /* make sure to check the fd we used to fill the gap */
                i--;

                continue;
            }
			char buffer[1024];
			int bytes_read = read(poll_fds[i].fd, buffer, sizeof(buffer));
			if (bytes_read > 0) {
				buffer[bytes_read] = '\0';
				if (sqlite3_exec(db, buffer, cb_send_results, &poll_fds[i].fd, &zErrMsg) != SQLITE_OK) {
					fprintf(stderr, "SQL error: %s\n", zErrMsg);
					sqlite3_free(zErrMsg);
				}
			}
		}
	}
	
	close(socket_desc);
	exit(EXIT_SUCCESS);

}
int main(int argc, char* argv[]) {

    char *ds = "domain_socket";
	server(ds);
}