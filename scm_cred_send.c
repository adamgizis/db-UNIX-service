#include "scm_cred.h"

int
main(int argc, char *argv[])
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
        sfd = unixConnect(SOCK_PATH, SOCK_STREAM);
    }

    /* Send real plus ancillary data */

    ssize_t ns = sendmsg(sfd, &msgh, 0);
    if (ns == -1)
        errExit("sendmsg");

    printf("sendmsg() returned %zd\n", ns);

    sleep(30);
    //exit(EXIT_SUCCESS);
}
