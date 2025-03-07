#include "scm_cred_send.h"

    int close_connection(int sfd) {
        if (sfd < 0) {
            return -1;  // Invalid socket
        }
    
        if (close(sfd) == -1) {
            perror("close");
            return -1;  // Error occurred while closing the socket
        }
    
        return 0;  // Socket closed successfully
    }

    // returns a proper file descriptor and sends aux data
    int client_connect(){
       union {
            char   buf[CMSG_SPACE(sizeof(struct ucred))];
                            /* Space large enough to hold a ucred structure */
            struct cmsghdr align;
        } controlMsg;

        /* The 'msg_name' field can be used to specify the address of the
        destination socket when sending a datagram. However, we do not
        need to use this field because we use connect() below, which sets
        a default outgoing address for datagrams. */

        struct msghdr msgh;
        msgh.msg_name = NULL;
        msgh.msg_namelen = 0;


        // Using just auxillary data
        int data =12345;

        struct iovec iov;
        iov.iov_base = &data;
        iov.iov_len = sizeof(data);
        msgh.msg_iov = &iov;
        msgh.msg_iovlen = 1;



        /* Set 'msgh' fields to describe the ancillary data buffer */

        msgh.msg_control = controlMsg.buf;
        msgh.msg_controllen = sizeof(controlMsg.buf);

        /* The control message buffer must be zero-initialized in order for the
            CMSG_NXTHDR() macro to work correctly. Although we don't need to use
            CMSG_NXTHDR() in this example (because there is only one block of
            ancillary data), we show this step to demonstrate best practice */

        memset(controlMsg.buf, 0, sizeof(controlMsg.buf));

        /* Set message header to describe the ancillary data that
            we want to send */

        struct cmsghdr *cmsgp = CMSG_FIRSTHDR(&msgh);
        cmsgp->cmsg_len = CMSG_LEN(sizeof(struct ucred));
        cmsgp->cmsg_level = SOL_SOCKET;
        cmsgp->cmsg_type = SCM_CREDENTIALS;

        /* Use sender's own PID, real UID, and real GID, unless
            alternate values were supplied on the command line */

        struct ucred creds;

        creds.pid = getpid();
        creds.uid = getuid();
        creds.gid = getgid();

        // printf("Send credentials pid=%ld, uid=%ld, gid=%ld\n",
                //(long) creds.pid, (long) creds.uid, (long) creds.gid);

        /* Copy 'ucred' structure into data field in the 'cmsghdr' */

        memcpy(CMSG_DATA(cmsgp), &creds, sizeof(struct ucred));


        /* Connect to the peer socket */

        int sfd;
        sfd = unixConnect(SOCK_PATH, SOCK_STREAM);
        while(sfd < 0){
            //printf("trying to connect...\n");
            sfd = unixConnect(SOCK_PATH, SOCK_STREAM);
        }

        /* Send real plus ancillary data */

        ssize_t ns = sendmsg(sfd, &msgh, 0);
        if (ns == -1){
            // printf("this is the error");
            errExit("sendmsg");
        }

        // printf("sendmsg() returned %zd\n", ns);

        /* Only send credentials once   */

        msgh.msg_control = NULL;
        msgh.msg_controllen = 0;

        return sfd;

    }


    int send_files(int sfd, int* fdList, int fdCnt, const char* json_string){
        
        // depends how you want to handle this
        if(fdCnt == 0){
            return -1;
        }


        size_t fdAllocSize = sizeof(int) * (fdCnt);
        size_t controlMsgSize = CMSG_SPACE(fdAllocSize);

        char *controlMsg = malloc(controlMsgSize);
        if (controlMsg == NULL){
            errExit("malloc");
            return -1;
        }

        memset(controlMsg, 0, controlMsgSize);
        
        struct msghdr msgh;
        msgh.msg_name = NULL;
        msgh.msg_namelen = 0;

        struct iovec iov;
        iov.iov_base = json_string;
        iov.iov_len = strlen(json_string);  
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
        memcpy(CMSG_DATA(cmsgp), fdList, fdAllocSize);

        if(cmsgp->cmsg_type == SCM_RIGHTS){
        // printf("its the msg_control!\n");    
        }

        ssize_t ns = sendmsg(sfd, &msgh, 0);
        if (ns == -1){
            errExit("sendmsg");
            return -1;
        }

        // printf("sendmsg() returned %zd\n", ns);
        return 0;

    }


    int* receive_fds(int socket, int *num_fds) {
        struct msghdr msgh;
        struct iovec iov;
        char buf[1];  // Dummy buffer for message data
        char control[CMSG_SPACE(sizeof(int) * MAX_FDS)];  // Control buffer to hold multiple FDs
    
        // Set up the message header
        memset(&msgh, 0, sizeof(msgh));
        iov.iov_base = buf;
        iov.iov_len = sizeof(buf);
        msgh.msg_iov = &iov;
        msgh.msg_iovlen = 1;
        msgh.msg_control = control;
        msgh.msg_controllen = sizeof(control);
    
        // Receive message
        if (recvmsg(socket, &msgh, 0) == -1) {
            perror("recvmsg");
            return NULL;
        }
    
        // Extract control message
        struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msgh);
        if (cmsg == NULL || cmsg->cmsg_level != SOL_SOCKET || cmsg->cmsg_type != SCM_RIGHTS) {
            fprintf(stderr, "Invalid control message\n");
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

 // returns a list of file descriptors
    int* get_articles(int sfd,int* ids, int* num_ids){
        if(sfd < 0 ){
            return NULL;
        }

        struct json_object *json_obj = json_object_new_object();
        json_object_object_add(json_obj, "action", json_object_new_string("GET_ARTICLE"));


        json_object *ids_json = json_object_new_array();
        for (int i = 0; i < *num_ids; ++i) {
            json_object_array_add(ids_json, json_object_new_int(ids[i])); 
        }                         
        json_object_object_add(json_obj, "ids", ids_json);
        
        const char *request = json_object_to_json_string_ext(json_obj, JSON_C_TO_STRING_PLAIN);
        if (write(sfd, request, strlen(request)) == -1) {
            perror("write");
            close(sfd);
            exit(EXIT_FAILURE);
            return NULL;
        }

        int *fds = receive_fds(sfd, num_ids);
        for(int i = 0; i < *num_ids; i++){
            // printf("%d", fds[i]);
        }
        return fds;
    }


// returns a list of file ids
// int* list_articles(int sfd){
// }

    // returns 0 on success, or an FD to error.txt on server error
    int delete_articles(int sfd, int* ids, int* num_ids) {
        if(sfd < 0) {
            return -1; // Return -1 for invalid socket
        }

        // Prepare the JSON request
        struct json_object *json_obj = json_object_new_object();
        json_object_object_add(json_obj, "action", json_object_new_string("DELETE_ARTICLE"));

        json_object *ids_json = json_object_new_array();
        for (int i = 0; i < *num_ids; ++i) {
            json_object_array_add(ids_json, json_object_new_int(ids[i])); 
        }
        json_object_object_add(json_obj, "ids", ids_json);

        const char *request = json_object_to_json_string_ext(json_obj, JSON_C_TO_STRING_PLAIN);

        // Send the request to the server
        if (write(sfd, request, strlen(request)) == -1) {
            perror("write");
            close(sfd);
            return -1;  // Return -1 if writing fails
        }

        return 0;
    }

    int upload_article(int sfd, const char* filepath, const char* title) {
        struct json_object *json_obj = json_object_new_object();
        if (!json_obj) {
            fprintf(stderr, "Failed to allocate JSON object\n");
            return -1;
        }
        
        json_object_object_add(json_obj, "action", json_object_new_string("UPLOAD_ARTICLES"));
    
        struct json_object *json_mapping = json_object_new_object();
        if (!json_mapping) {
            fprintf(stderr, "Failed to allocate JSON mapping object\n");
            json_object_put(json_obj);
            return -1;
        }
        json_object_object_add(json_obj, "file_mapping", json_mapping);
    
        int fd = open(filepath, O_RDWR);
        if (fd < 0) {
            perror("open");
            json_object_put(json_obj);
            close(sfd);
            return -1;
        }
    
        char file_descriptor[12]; // Increased size to safely hold int as string
        snprintf(file_descriptor, sizeof(file_descriptor), "%d", fd);
    
        json_object_object_add(json_mapping, file_descriptor, json_object_new_string(title));
    
        const char *request = json_object_to_json_string_ext(json_obj, JSON_C_TO_STRING_PLAIN);
    
        struct iovec iov;
        struct msghdr msgh;
        char buffer[1024]; // Ensure buffer is large enough
    
        memset(&iov, 0, sizeof(iov));
        memset(&msgh, 0, sizeof(msgh));
    
        iov.iov_base = (void *)request;
        iov.iov_len = strlen(request) + 1;
    
        msgh.msg_iov = &iov;
        msgh.msg_iovlen = 1;
        msgh.msg_control = NULL;
        msgh.msg_controllen = 0;
    
        int bytes_sent = send_files(sfd, &fd, 1, request);
        if (bytes_sent == -1) {
            perror("sendmsg");
            close(fd);
            json_object_put(json_obj);
            return -1;
        }
    
        iov.iov_base = buffer;
        iov.iov_len = sizeof(buffer);
    
        int bytes_received = recvmsg(sfd, &msgh, 0);
        if (bytes_received == -1) {
            perror("recvmsg");
            close(fd);
            json_object_put(json_obj);
            return -1;
        }
    
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            //printf("%s\n", buffer);
        }
    
        // Clean up
        // close(fd);
        json_object_put(json_obj);
    
        return 0;
    }

    // Function to receive JSON message over the socket and return a json_object
    struct json_object* receive_json(int sfd) {
        char buffer[BUFFER_SIZE];
        ssize_t bytes_received;
    
        // Read the message from the socket
        bytes_received = recv(sfd, buffer, sizeof(buffer) - 1, 0);
        
        if (bytes_received < 0) {
            perror("recv failed");
            return NULL;
        } else if (bytes_received == 0) {
            // printf("Connection closed by server.\n");
            return NULL;
        }
    
        buffer[bytes_received] = '\0';  // Null-terminate the received data
        //printf("Received JSON: %s\n", buffer);
    
        // Now, parse the JSON
        struct json_object *parsed_json = json_tokener_parse(buffer);
        if (!parsed_json) {
            fprintf(stderr,"Failed to parse JSON.\n");
            return NULL;
        }
    
        // Pretty-print the JSON
        //printf("Parsed JSON: %s\n", json_object_to_json_string_ext(parsed_json, JSON_C_TO_STRING_PRETTY));
    
        
        return parsed_json;
    }
    
    struct json_object* list_articles(int sfd){
        if(sfd < 0 ){
            return NULL;
        }

        struct json_object *json_obj = json_object_new_object();
        json_object_object_add(json_obj, "action", json_object_new_string("LIST_ARTICLES"));


        const char *request = json_object_to_json_string_ext(json_obj, JSON_C_TO_STRING_PLAIN);


        if (write(sfd, request, strlen(request)) == -1) {
            perror("write");
            close(sfd);
            exit(EXIT_FAILURE);
            return NULL;
        }

        json_object_put(json_obj);

        return receive_json(sfd);
    }
