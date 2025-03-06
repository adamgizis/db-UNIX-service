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
    int article_fds[MAX_FDS];
    int num_fds;

} query_context_t;



// int* fdList has to be a list of open fds
//    int *fdList = malloc(fdAllocSize);
/*
code to use if filepaths is the path to all of the files that need to be opened.
if (fdList == NULL)
errExit("calloc");

for (int j = 0; j < fdCnt; j++) {
 fdList[j] = open(filepaths[j], O_RDONLY);
 if (fdList[j] == -1)
     errExit("open");
}
*/

int* receive_fds(struct msghdr *msgh, int *num_fds) {

    struct cmsghdr *cmsg;
    int found_fd = 0;
    for (cmsg = CMSG_FIRSTHDR(msgh); cmsg != NULL; cmsg = CMSG_NXTHDR(msgh, cmsg)) {
        if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_RIGHTS) {
            // Process SCM_RIGHTS message
            found_fd = 1;
            break;
        }
    }
    if (!found_fd) {
        fprintf(stderr, "No SCM_RIGHTS message received\n");
        return NULL;
    }

    // Determine number of FDs received
    *num_fds = (cmsg->cmsg_len - CMSG_LEN(0)) / sizeof(int);
    if (*num_fds <= 0) {
        fprintf(stderr, "No file descriptors received\n");
        return NULL;
    }

    // Allocate memory for FD list
    int *fds = malloc(*num_fds * sizeof(int));
    if (!fds) {
        perror("malloc");
        return NULL;
    }

    // Copy received FDs
    memcpy(fds, CMSG_DATA(cmsg), *num_fds * sizeof(int));

    return fds;  // Caller is responsible for freeing this
}

int send_files(query_context_t *context){

    printf("send files\n");
    
    // depends how you want to handle this
    if(context->num_fds == 0){
        return -1;
    }


    size_t fdAllocSize = sizeof(int) * (context->num_fds);
    size_t controlMsgSize = CMSG_SPACE(fdAllocSize);

    char *controlMsg = malloc(controlMsgSize);
    if (controlMsg == NULL){
        free(context);
        errExit("malloc");
        return -1;
    }


    memset(controlMsg, 0, controlMsgSize);
    
    struct msghdr msgh;
    msgh.msg_name = NULL;
    msgh.msg_namelen = 0;


        // dummy variable
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


    // see code above function
    memcpy(CMSG_DATA(cmsgp), context->article_fds, fdAllocSize);

    ssize_t ns = sendmsg(context->client_socket, &msgh, 0);
    if (ns == -1){
        free(context);
        errExit("sendmsg");
        return -1;
    }

    printf("sendmsg() returned %zd\n", ns);
    return 0;

}


void send_file(void* context){

    query_context_t *ctx = (query_context_t *)context;  
    int sfd = ctx->client_socket;
    //char * path = ctx->filepath;
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
        //fdList[j] = open(path, O_RDONLY);
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

    if (write(error_fd, error_msg, strlen(error_msg)) < 0) {
        perror("Write to file descriptor failed");
        return 1;
    }

    send_file(&context);

    return 0;
}


static int cb_send_fds(void *context, int argc, char **argv, char **azColName) {
    printf("cb_send_fds\n");
    (void)azColName;
    query_context_t *ctx = (query_context_t *)context;  
    
    for (int i = 0; i < argc; i++) {
        ctx->article_fds[i] = open(argv[i], O_RDONLY);
        if (ctx->article_fds[i] == -1){
            errExit("invalid file path in database: %s", argv[i]);
        }
    } 
    
    ctx->num_fds += argc;

    return 0;
}

/*
static int cb_send_results_multiple_fds(void *context, int argc, char **argv, char **azColName) {
    printf("cb_send_results_multiple: %d results\n", argc);
    query_context_t *ctx = (query_context_t *)context;
    int fds[argc];
    int client_fd = ctx->client_socket;
    
    for (int i = 0; i < argc; i++) {
        printf("%s\n", argv[i]);
        fds[i] = open(argv[i], O_RDONLY);
        if (fds[i] == -1){
            errExit("invalid file path in database: %s", argv[i]);
        }
    }                                             

    send_files(client_fd, fds, argc);

    return 0;
}
*/

int format_ids(struct json_object *array, char *query) {
    if (json_object_get_type(array) != json_type_array) {
        return -1;
    }

    int num_ids = json_object_array_length(array);
    for(int i = 0; i < num_ids; i++) {
        struct json_object *id = json_object_array_get_idx(array, i);
        char buffer[64];
        if (i < num_ids - 1)
            snprintf(buffer, sizeof(buffer), "%d, ", json_object_get_int(id));
        else
            snprintf(buffer, sizeof(buffer), "%d", json_object_get_int(id));

        strcat(query, buffer);
    }

    return num_ids;
}


/*
void execute_query_and_send(sqlite3 *db, const char *query, int client_fd, int num_fds) {
    char *zErrMsg = NULL;
    query_context_t context;
    context.client_socket = client_fd;

    printf("Executing: %s\n", query);

    if(num_fds > 1) {
        if (sqlite3_exec(db, query, cb_send_results_multiple_fds, &context, &zErrMsg) != SQLITE_OK) {
            char error_msg[512];
            snprintf(error_msg, sizeof(error_msg), "SQL error: %s\n", zErrMsg);
            write(context.output_fd, error_msg, strlen(error_msg));
            sqlite3_free(zErrMsg);
        }
    } else{
        if (sqlite3_exec(db, query, cb_send_results, &context, &zErrMsg) != SQLITE_OK) {
            char error_msg[512];
            snprintf(error_msg, sizeof(error_msg), "SQL error: %s\n", zErrMsg);
            write(context.output_fd, error_msg, strlen(error_msg));
            sqlite3_free(zErrMsg);
        }
    }                   

}
    */

void execute_query_and_send_fds(sqlite3 *db, const char *query, int client_fd) {
    char *zErrMsg = NULL;
    query_context_t *context = calloc(1, sizeof(query_context_t));
    context->client_socket = client_fd;
    context->num_fds = 0;

    if (sqlite3_exec(db, query, cb_send_fds,    context, &zErrMsg) != SQLITE_OK) {
        char error_msg[512];
        snprintf(error_msg, sizeof(error_msg), "SQL error: %s\n", zErrMsg);
        send_error(error_msg, client_fd);
        sqlite3_free(zErrMsg);
    }

    send_files(context);
    free(context);                      
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

sqlite3* initiate_db(sqlite3* db) {
    if (sqlite3_open(DB_PATH, &db) != SQLITE_OK) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return NULL;
    }
    return db;
}

int process_client_request(Client *c) {
    struct msghdr msgh = c->msgh;
    struct iovec iov = c->iov;
    char buffer[BUFFER_SIZE] = {0};
    char control[CMSG_SPACE(sizeof(int) * MAX_FDS)];  // Control buffer to hold multiple FDs

    // Set up the message header    
    memset(&msgh, 0, sizeof(msgh));
    iov.iov_base = buffer;
    iov.iov_len = sizeof(buffer);
    msgh.msg_iov = &iov;
    msgh.msg_iovlen = 1;
    msgh.msg_control = control;
    msgh.msg_controllen = sizeof(control);


    int bytes_read;
    if ((bytes_read = recvmsg(c->pollfd.fd, &msgh, 0)) == -1) {
        perror("recvmsg");
        return -1;
    }

    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';      

        struct json_object *json_req = json_tokener_parse(buffer);
        if (!json_req) {
            char *error_msg = "Invalid JSON";
            send_error(error_msg, c->pollfd.fd);
            return -1;
        }

        struct json_object *json_obj;
        if (!json_object_object_get_ex(json_req, "action", &json_obj)) {
            send_error("Missing action field", c->pollfd.fd);
            json_object_put(json_req);
            return -1;
        }
        
        const char *request = json_object_get_string(json_obj);                                       

        if (strcmp(request, "LIST_ARTICLES") == 0) {
            printf("LIST_ARTICLES\n");
            json_object_put(json_req);
            const char *query = "SELECT articles.id, title, username "
                                           "FROM articles INNER JOIN users "
                                           "ON articles.author_id = users.id "
                                           "WHERE is_published = TRUE;";
            // NEED TO MAKE IT SEND AS JSON STRING IN IOV
            //execute_query_and_send(db, query, c->pollfd.fd, 0);

        } else if (strcmp(request, "GET_ARTICLE") == 0) {
            printf("GET_ARTICLES\n");
            if (!json_object_object_get_ex(json_req, "ids", &json_obj)) {
                send_error("No article ids given", c->pollfd.fd);
                json_object_put(json_req);
                return -1;
            }

            char query[BUFFER_SIZE] = {0};
            strcpy(query, "SELECT file_path FROM articles WHERE id IN (");

            int num_ids;
            if ((num_ids = format_ids(json_obj, query)) < 0) {
                send_error("ids must be given as a JSON array", c->pollfd.fd);
                json_object_put(json_req);
                return -1;
            }

            strcat(query, ");");

            json_object_put(json_req);
            execute_query_and_send_fds(db, query, c->pollfd.fd);

        } else if (strcmp(request, "UPLOAD_ARTICLES") == 0){
            printf("UPLOAD_ARTICLES\n");
            int num_fds;
            int *fds;
            fds = receive_fds(&msgh, &num_fds);
            if(!fds){
                return -1;
            }

            struct json_object *file_mapping;
            if (!json_object_object_get_ex(json_req, "file_mapping", &file_mapping)) {
                fprintf(stderr, "\"file_mapping\" not found in JSON\n");
                return -1;
            }
            
            json_object_object_foreach(file_mapping, key, val) {
                int src_fd = atoi(key);
                const char *filename = json_object_get_string(val);

                printf("File descriptor: %d, Filename: %s\n", src_fd, filename);

                char file_path[BUFFER_SIZE];
                snprintf(file_path, sizeof(file_path), "%s%s.md", WIKI_PATH, filename);

                int wiki_fd = open(file_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if(wiki_fd < 0){
                    close(src_fd);
                    send_error("invalid filename", c->pollfd.fd);
                }

                off_t offset = 0;
                struct stat stat_buf;
                fstat(src_fd, &stat_buf);

                ssize_t bytes_copied = sendfile(wiki_fd, src_fd, &offset, stat_buf.st_size);
                if (bytes_copied == -1) {
                    send_error("Error during sendfile", c->pollfd.fd);
                } 

                close(src_fd);
                close(wiki_fd);

                /*
                    NOT WORKING
                char query[BUFFER_SIZE];
                snprintf(query, sizeof(query), "INSERT INTO articles (title, file_path, author_id, is_published) "
                                                "VALUES ( "
                                                   "'%s', "
                                                   "'%s',"
                                                    "%d,"
                                                   "1);", json_object_get_string(val), file_path, c->creds.uid);

                                                   
                char *zErrMsg = NULL;
                if(sqlite3_exec(db, query, NULL, NULL, &zErrMsg) != SQLITE_OK) {
                    char error_msg[512];
                    snprintf(error_msg, sizeof(error_msg), "SQL error: %s\n", zErrMsg);
                    send_error(error_msg, c->pollfd.fd);
                    sqlite3_free(zErrMsg);
                }
                    */
            }

        } else if (strcmp(request, "DELETE_ARTICLE") == 0){
          // NEED TO ADD ABILITY TO DELETE (ONLY ADMIN OR IF USER IS AUTHOR)
            printf("DELETE_ARTICLE");

            // Ensure "ids" is present in the JSON request
            if (!json_object_object_get_ex(json_req, "ids", &json_obj)) {
                send_error("No article ids given", c->pollfd.fd);
                json_object_put(json_req);
                return -1;
            }

            // Start building the SQL query safely
            const char *base_query = (c->creds.uid != 0) ?
                "DELETE FROM articles WHERE author_id = ? AND id IN (" :
                "DELETE FROM articles WHERE id IN (";

            char query[BUFFER_SIZE] = {0};
            snprintf(query, sizeof(query), "%s", base_query);

            // Format the article IDs into the query
            int num_ids = format_ids(json_obj, query);
            if (num_ids < 0) {
                send_error("ids must be given as a JSON array", c->pollfd.fd);
                json_object_put(json_req);
                return -1;
            }

            strcat(query, ");"); // Close the IN() clause

            // Prepare statement
            sqlite3_stmt *stmt;
            if (sqlite3_prepare_v2(db, query, -1, &stmt, NULL) != SQLITE_OK) {
                fprintf(stderr, "SQL Error: %s\n", sqlite3_errmsg(db));
                send_error("Failed to prepare delete query", c->pollfd.fd);
                json_object_put(json_req);
                return -1;
            }

            // Bind parameters (only bind UID if needed)
            if (c->creds.uid != 0) {
                sqlite3_bind_int(stmt, 1, c->creds.uid); // Bind user ID if needed
            }

            // Execute delete query
            if (sqlite3_step(stmt) != SQLITE_DONE) {
                fprintf(stderr, "Deletion failed: %s\n", sqlite3_errmsg(db));
                send_error("Failed to delete articles", c->pollfd.fd);
                sqlite3_finalize(stmt);
                json_object_put(json_req);
                return -1;
            } else {
                int affected_rows = sqlite3_changes(db); // Get the number of affected rows
                if (affected_rows > 0) {
                    send_error("Successfully deleted articles.", c->pollfd.fd);
                } else {
                    send_error("No articles matched the criteria.", c->pollfd.fd);
                }
            }

            // Cleanup
            sqlite3_finalize(stmt);
            json_object_put(json_req);
        }
    }
    return 0;
}

void server(){

    // Initialize the db
    db = initiate_db(db);  

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

            

                    struct passwd *pw = getpwuid(c->creds.uid);
                    
                    char query[BUFFER_SIZE];
                    snprintf(query, sizeof(query), "INSERT OR IGNORE INTO users (id, username, role)"
                                                    "VALUES (%d, '%s', 'user');", c->creds.uid, pw->pw_name);

                    char *zErrMsg = NULL;
                    if(sqlite3_exec(db, query, NULL, NULL, &zErrMsg) != SQLITE_OK) {
                        char error_msg[512];
                        snprintf(error_msg, sizeof(error_msg), "SQL error: %s\n", zErrMsg);
                        send_error(error_msg, c->pollfd.fd);
                        sqlite3_free(zErrMsg);
                    }
            
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
                process_client_request(c);
            }
        }
    }
}

int main(){
    server();
}

