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
#include <fcntl.h>

#define DB_PATH "database/regs.db"
#define MAX_FDS 16

typedef struct {
    int client_socket;
    int output_fd;
} query_context_t;

static int cb_send_results(void *context, int argc, char **argv, char **azColName) {
    query_context_t *ctx = (query_context_t *)context;  
    char buffer[1024];  
    buffer[0] = '\0';  
    
    for (int i = 0; i < argc; i++) {
        snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), 
                 "%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    strcat(buffer, "\n");  
    
    if (write(ctx->output_fd, buffer, strlen(buffer)) < 0) {
        perror("Write to file descriptor failed");
        return 1;
    }
    return 0;
}

void execute_query_and_send(sqlite3 *db, const char *query, int client_fd, int output_fd) {
    char *zErrMsg = NULL;
    query_context_t context = {client_fd, output_fd};

    if (sqlite3_exec(db, query, cb_send_results, &context, &zErrMsg) != SQLITE_OK) {
        char error_msg[512];
        snprintf(error_msg, sizeof(error_msg), "SQL error: %s\n", zErrMsg);
        write(output_fd, error_msg, strlen(error_msg));
        sqlite3_free(zErrMsg);
    }
}

sqlite3* initiate_db() {
    sqlite3 *db;
    if (sqlite3_open(DB_PATH, &db) != SQLITE_OK) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return NULL;
    }
    return db;
}

void server(char *filename) {
    sqlite3* db = initiate_db();
    if (!db) exit(EXIT_FAILURE);
    
    struct pollfd poll_fds[MAX_FDS];
    int socket_desc, num_fds = 0;
    socket_desc = domain_socket_server_create(filename);

    memset(poll_fds, 0, sizeof(poll_fds));
    poll_fds[0].fd = socket_desc;
    poll_fds[0].events = POLLIN;
    num_fds++;

    for (;;) {
        int ret = poll(poll_fds, num_fds, -1);
        if (ret == -1) panic("poll error");
        
        if (poll_fds[0].revents & POLLIN) {
            int new_client = accept(socket_desc, NULL, NULL);
            if (new_client == -1) panic("server accept");
            
            if (num_fds < MAX_FDS) {
                poll_fds[num_fds].fd = new_client;
                poll_fds[num_fds].events = POLLIN;
                num_fds++;
                printf("server: created client connection %d\n", new_client);
            } else {
                close(new_client);
            }
        }
        
        for (int i = 1; i < num_fds; i++) {
            if (poll_fds[i].revents & (POLLHUP | POLLERR)) {
                printf("server: closing client connection %d\n", poll_fds[i].fd);
                close(poll_fds[i].fd);
                poll_fds[i] = poll_fds[num_fds - 1];
                num_fds--;
                i--;
                continue;
            }
            
            if (poll_fds[i].revents & POLLIN) {
                char buffer[1024];
                int bytes_read = read(poll_fds[i].fd, buffer, sizeof(buffer) - 1);
                if (bytes_read > 0) {
                    buffer[bytes_read] = '\0';
                    int query_fd = open("query_results.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (query_fd < 0) {
                        perror("Failed to open query file");
                        continue;
                    }
                    execute_query_and_send(db, buffer, poll_fds[i].fd, query_fd);
                    close(query_fd);
                }
            }
        }
    }
    close(socket_desc);
    sqlite3_close(db);
}

int main(int argc, char* argv[]) {
    server("domain_socket");
}
