#include "scm_cred.h"


int recv_fd(int socket) {
    struct msghdr msgh;
    msgh.msg_name = NULL;
    msgh.msg_namelen = 0;
    
    // Control message buffer
    // single 
    size_t cmsgbuf[CMSG_SPACE(sizeof(int))];
    memset(cmsgbuf, 0, sizeof(cmsgbuf));

    // dummy
    struct iovec iov;
    int data = 12345;
    iov.iov_base = &data;
    iov.iov_len = sizeof(data);
    msgh.msg_iov = &iov;
    msgh.msg_iovlen = 1;

    // Set up control message
    msgh.msg_control = cmsgbuf;
    msgh.msg_controllen = sizeof(cmsgbuf);

    // recieve
    if (recvmsg(socket, &msgh, 0) == -1) {
        perror("recvmsg");
        return -1;
    }

    // Extract file descriptor
    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msgh);
    if (cmsg == NULL || cmsg->cmsg_level != SOL_SOCKET || cmsg->cmsg_type != SCM_RIGHTS) {
        fprintf(stderr, "Invalid control message\n");
        return -1;
    }   

    int fd;
    memcpy(&fd, CMSG_DATA(cmsg), sizeof(int));
    return fd;
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
    const char *query = "CREATE TABLE employees (employee_id INT PRIMARY KEY,first_name VARCHAR(50),last_name VARCHAR(50),department VARCHAR(50),salary DECIMAL(10, 2));";
    if (write(sfd, query, strlen(query)) == -1) {
        perror("write");
        close(sfd);
        exit(EXIT_FAILURE);
    }
    printf("Query: %s\n", query);

    // Receive the file descriptor
    int received_fd = recv_fd(sfd);
    if (received_fd == -1) {
        close(sfd);
        errExit("Can't Read file descriptor");
    }
    //Verify the received FD
    if (fcntl(received_fd, F_GETFD) == -1) {
        close(sfd);
        errExit("Invalid file descriptor");
    }
    printf("Received file descriptor: %d\n", received_fd);

    ssize_t bytes_read;
    char buffer[1024];
        
    while((bytes_read = read(received_fd, buffer, sizeof(buffer))) > 0) {
        write(STDOUT_FILENO, buffer, bytes_read);
    }


    sleep(30);
    //exit(EXIT_SUCCESS);
}
