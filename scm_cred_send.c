    #include "scm_cred.h"

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
    
    int
    main()
    {
        /* Allocate a char array of suitable size to hold the ancillary data.
        However, since this buffer is in reality a 'struct cmsghdr', use a
        union to ensure that it is aligned as required for that structure.
        Alternatively, we could allocate the buffer using malloc(), which
        returns a buffer that satisfies the strictest alignment
        requirements of any type */

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

        printf("Send credentials pid=%ld, uid=%ld, gid=%ld\n",
                (long) creds.pid, (long) creds.uid, (long) creds.gid);

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
        if (ns == -1)
            errExit("sendmsg");

        printf("sendmsg() returned %zd\n", ns);

        /* Only send credentials once   */

        msgh.msg_control = NULL;
        msgh.msg_controllen = 0;

        //ssize_t bytes_read;
        //char buffer[1024];

        // // Read at most BUF_SIZE bytes from STDIN into buf.
        // while ((bytes_read = read(STDIN_FILENO, buffer, sizeof(buffer))) > 0) {
        //     // Then, write those bytes from buf into the socket.

        //     iov.iov_base = buffer;  
        //     iov.iov_len = bytes_read;

        //     ssize_t ns = sendmsg(sfd, &msgh, 0);
        //     if (ns == -1)
        //         errExit("sendmsg");
        
        //     printf("sendmsg() returned %zd\n", ns);
        // }
        
        // if (bytes_read == -1) {
        //     errExit("read");
        // }
        
        // Test Query
        //const char *query = "SELECT * FROM users";

        struct json_object *json_obj = json_object_new_object();
        json_object_object_add(json_obj, "action", json_object_new_string("GET_ARTICLES"));

        const char *request = json_object_to_json_string_ext(json_obj, JSON_C_TO_STRING_PLAIN);

        if (write(sfd, request, strlen(request)) == -1) {
            perror("write");
            close(sfd);
            exit(EXIT_FAILURE);
        }
        printf("Query: %s\n", request);

        printf("recieving fd\n");
        // Receive the file descriptor
        int num_fds;
    
        int *fds = receive_fds(sfd, &num_fds);
    
        printf("verifying fd\n");
        //Verify the received FD
        if (fds) {
            printf("Received %d file descriptors:\n", num_fds);
            for (int i = 0; i < num_fds; i++) {
                printf("Recieved %d\n", fds[i]);
                ssize_t bytes_read;
                char buffer[1024];
                    
                printf("printing results\n");   
                while((bytes_read = read(fds[i], buffer, sizeof(buffer))) > 0) {
                    write(STDOUT_FILENO, buffer, bytes_read);
                }
            }
            free(fds);
        } else {
            printf("Failed to receive file descriptors\n");
            
        }

        // ssize_t bytes_read;
        // char buffer[1024];
            
        // printf("printing results\n");   
        // while((bytes_read = read(received_fd, buffer, sizeof(buffer))) > 0) {
        //     write(STDOUT_FILENO, buffer, bytes_read);
        // }


        sleep(30);
        //exit(EXIT_SUCCESS);
    }
