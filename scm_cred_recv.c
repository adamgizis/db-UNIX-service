/* scm_cred_recv.c

   See also scm_multi_recv.c.
*/
#include "scm_cred.h"

typedef struct {
    struct pollfd pollfd;  // Stores fd and events for poll()
    struct msghdr msgh;    // Message header for receiving ancillary data
    struct iovec iov;      // Buffer for received data
    struct ucred creds;    // Stores credentials received from the client
    char control_buf[CMSG_SPACE(sizeof(struct ucred))]; // Ancillary data buffer
    int data;              // Received data
} Client;


sqlite3* db;

#define SUDO = "sudo"

typedef struct {
    int client_socket;
    int output_fd;
    char* filepath;
} query_context_t;


void send_file(void * context){

    query_context_t *ctx = (query_context_t *)context;  
    int sfd = ctx->client_socket;
    char * path = ctx->filepath;
    // always just sending the one file for now
    size_t fdAllocSize = sizeof(int);
    size_t controlMsgSize = CMSG_SPACE(fdAllocSize);
    char *controlMsg = malloc(controlMsgSize);
    if (controlMsg == NULL)
        errExit("malloc");

    /* The control message buffer must be zero-initialized in order for
       the CMSG_NXTHDR() macro to work correctly */

    memset(controlMsg, 0, controlMsgSize);

    /* The 'msg_name' field can be used to specify the address of the
       destination socket when sending a datagram. However, we do not
       need to use this field because we use connect() below, which sets
       a default outgoing address for datagrams. */

    struct msghdr msgh;
    msgh.msg_name = NULL;
    msgh.msg_namelen = 0;
    
    // dummy
    struct iovec iov;
    int data = 12345;
    iov.iov_base = &data;
    iov.iov_len = sizeof(data);
    msgh.msg_iov = &iov;
    msgh.msg_iovlen = 1;

    /* Place a pointer to the ancillary data, and size of that data,
       in the 'msghdr' structure that will be passed to sendmsg() */

    msgh.msg_control = controlMsg;
    msgh.msg_controllen = controlMsgSize;

    /* Set message header to describe the ancillary data that
       we want to send */

    /* First, the file descriptor list */

    struct cmsghdr *cmsgp = CMSG_FIRSTHDR(&msgh);
    cmsgp->cmsg_level = SOL_SOCKET;
    cmsgp->cmsg_type = SCM_RIGHTS;

    /* The ancillary message must include space for the required number
       of file descriptors */

    cmsgp->cmsg_len = CMSG_LEN(fdAllocSize);

    /* Open files named on the command line, and copy the resulting block of
       file descriptors into the data field of the ancillary message */
    int fdCnt = 1;
    int *fdList = malloc(fdAllocSize);
    if (fdList == NULL)
        errExit("calloc");

    /* Open the files named on the command line, placing the returned file
       descriptors into the ancillary data */

    for (int j = 0; j < fdCnt; j++) {
        fdList[j] = open(path, O_RDONLY);
        if (fdList[j] == -1)
            errExit("open");
    }

    memcpy(CMSG_DATA(cmsgp), fdList, fdAllocSize);

    ssize_t ns = sendmsg(sfd, &msgh, 0);
    if (ns == -1)
        errExit("sendmsg");
}

static int send_error(const char *error_msg, int client_fd) {
    int error_fd = open("error.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    query_context_t context;
    context.client_socket = client_fd;
    context.output_fd = error_fd;
    context.filepath = "error.txt";

    if (write(error_fd, error_msg, strlen(error_msg)) < 0) {
        perror("Write to file descriptor failed");
        return 1;
    }

    send_file(&context);

    return 0;
}

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

    send_file((void*) context);

    return 0;
}

void execute_query_and_send(sqlite3 *db, const char *query, int client_fd) {
    char *zErrMsg = NULL;
    int query_fd = open("query.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    query_context_t context;
    context.client_socket = client_fd;
    context.output_fd = query_fd;
    context.filepath = "query.txt";

    if (sqlite3_exec(db, query, cb_send_results, &context, &zErrMsg) != SQLITE_OK) {
        char error_msg[512];
        snprintf(error_msg, sizeof(error_msg), "SQL error: %s\n", zErrMsg);
        write(context.output_fd, error_msg, strlen(error_msg));
        sqlite3_free(zErrMsg);
    }

}

int is_query_read_only(sqlite3 *db, const char *query) {
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, query, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        // Handle error
    }
    int is_read_only = sqlite3_stmt_readonly(stmt);

    sqlite3_finalize(stmt);

    return is_read_only;
}

sqlite3* initiate_db() {
    sqlite3 *db;
    if (sqlite3_open(DB_PATH, &db) != SQLITE_OK) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return NULL;
    }
    return db;
}

void server(){

    // Initialize the db
    db = initiate_db();

    if(!db) {
        errExit("initialize_db");
    }

    char *group_name = "sudo";
    struct group *sudo_grp = getgrnam(group_name);

    if (!sudo_grp) {
        errExit("getgrnam-%s", group_name);
    }

    if (remove(SOCK_PATH) == -1 && errno != ENOENT)
        errExit("remove-%s", SOCK_PATH);

    int num_clients = 0;
    int lfd = unixBind(SOCK_PATH, SOCK_STREAM);
    if (lfd == -1)
        errExit("unixBind");

    if (chmod(SOCK_PATH, 0777) == -1) 
            errExit("chmod");

    if (listen(lfd, 5) == -1)
        errExit("listen");

    Client clients[MAX_FDS] = {0};
    struct pollfd pollfds[MAX_FDS] = {0};

    // Initialize the listening socket
    clients[0].pollfd.fd = lfd;
    clients[0].pollfd.events = POLLIN;
    pollfds[0] = clients[0].pollfd;
    num_clients++;


    printf("starting polling...\n");

    // Polling loop
    for (;;) {
        int ret = poll(pollfds, num_clients, -1);
        if (ret == -1)
            errExit("poll error");

        for (int i = 0; i < num_clients; i++) {
            clients[i].pollfd = pollfds[i];
        }

        // Check for new connections
        if (clients[0].pollfd.revents & POLLIN) {
            int new_client = accept(lfd, NULL, NULL);
            if (new_client == -1)
                errExit("accept");

            if (num_clients < MAX_FDS) {
                // Add new client
                Client *c = &clients[num_clients];
                c->pollfd.fd = new_client;
                c->pollfd.events = POLLIN;
                pollfds[num_clients] = c->pollfd;

                int optval = 1;
                if (setsockopt(new_client, SOL_SOCKET, SO_PASSCRED, &optval, sizeof(optval)) == -1)
                    errExit("setsockopt");

                // Initialize msghdr
                c->iov.iov_base = &c->data;
                c->iov.iov_len = sizeof(int);
                c->msgh.msg_iov = &c->iov;
                c->msgh.msg_iovlen = 1;
                c->msgh.msg_control = c->control_buf;
                c->msgh.msg_controllen = sizeof(c->control_buf);



                ssize_t nr = recvmsg(c->pollfd.fd, &c->msgh, 0);
                if (nr == -1)
                    errExit("recvmsg");

                printf("recvmsg() returned %zd, received data = %d\n", nr, c->data);

                struct cmsghdr *cmsgp = CMSG_FIRSTHDR(&c->msgh);
                if (!cmsgp || cmsgp->cmsg_len != CMSG_LEN(sizeof(struct ucred)) ||
                    cmsgp->cmsg_level != SOL_SOCKET || cmsgp->cmsg_type != SCM_CREDENTIALS)
                    fatal("Invalid credentials received");

                struct ucred rcred;

                memcpy(&rcred, CMSG_DATA(cmsgp), sizeof(struct ucred));
                printf("Received credentials: pid=%ld, uid=%ld, gid=%ld\n",
                        (long)rcred.pid, (long)rcred.uid, (long)rcred.gid);

                socklen_t len = sizeof(struct ucred);
                if (getsockopt(new_client, SOL_SOCKET, SO_PEERCRED, &c->creds, &len) == -1)
                    errExit("getsockopt");

                printf("Credentials from SO_PEERCRED: pid=%ld, uid=%ld, gid=%ld\n",
                    (long)c->creds.pid, (long)c->creds.uid, (long)c->creds.gid);

                num_clients++;
                printf("server: created client connection %d\n", new_client);
            } else {
                close(new_client);
            }
        }
        // Check existing clients
        for (int i = 1; i < num_clients; i++) {
            Client *c = &clients[i];
            if (c->pollfd.revents & (POLLHUP | POLLERR)) {
                
                printf("server: closing client connection %d\n", c->pollfd.fd);
                close(c->pollfd.fd);
                
                // Replace the disconnected client with the last one in the array
                clients[i] = clients[num_clients - 1];
                pollfds[i] = pollfds[num_clients - 1];      
                num_clients--;

                memset(&clients[num_clients], 0, sizeof(Client));
        
                i--;
        
                continue;
            }

            if (c->pollfd.revents & POLLIN) { 
                
                char buffer[1024];
			    int bytes_read = read(c->pollfd.fd, buffer, sizeof(buffer));
			    if (bytes_read > 0) {
				    buffer[bytes_read] = '\0';
                    if(is_query_read_only(db, buffer) && (c->creds.gid != sudo_grp->gr_gid)) {
                        execute_query_and_send(db, buffer, c->pollfd.fd);
                    } else {
                        char *error_msg = "Invalid credentials for database modification";
                        send_error(error_msg, c->pollfd.fd);
                    }
			    }   
            }
        }
   }
}
int main(){
    server();
}

