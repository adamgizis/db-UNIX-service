#include "scm_cred.h"

#ifndef SCM_CRED_RECV_H
#define SCM_CRED_RECV_H

typedef struct {
    struct pollfd pollfd;
    struct msghdr msgh;
    struct iovec iov;
    struct ucred creds;
    char control_buf[CMSG_SPACE(sizeof(struct ucred))];
    int data;
} Client;

typedef struct {
    int client_socket;
    int article_fds[MAX_FDS];
    int num_fds;
} query_context_t;


int* extract_fds(struct msghdr *msgh, int *num_fds);
int send_files(query_context_t *context);
void send_json(const char *message, int client_fd);
static int send_error(const char *error_msg, int client_fd);
static int cb_send_fds(void *context, int argc, char **argv, char **azColName);
static int cb_send_json(void *json_array, int argc, char **argv, char **azColName);
int format_ids(struct json_object *array, char *query);
void execute_query_and_send_json(sqlite3 *db, const char *query, int client_fd);
void execute_query_and_send_fds(sqlite3 *db, const char *query, int client_fd);
int is_query_read_only(sqlite3 *db, const char *query);
sqlite3* initiate_db();
int process_client_request(Client *c);
void server();

#endif // SCM_CRED_RECV_H