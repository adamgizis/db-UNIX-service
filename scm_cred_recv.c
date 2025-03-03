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

int main(int argc, char *argv[]) {
    if (remove(SOCK_PATH) == -1 && errno != ENOENT)
        errExit("remove-%s", SOCK_PATH);

    int num_clients = 0;
    int lfd = unixBind(SOCK_PATH, SOCK_STREAM);
    if (lfd == -1)
        errExit("unixBind");

    if (listen(lfd, 5) == -1)
        errExit("listen");

    Client clients[MAX_FDS] = {0};

    // Initialize the listening socket
    clients[0].pollfd.fd = lfd;
    clients[0].pollfd.events = POLLIN;
    num_clients++;

    // Polling loop
    for (;;) {
        int ret = poll((struct pollfd *)clients, num_clients, -1);
        if (ret == -1)
            errExit("poll error");

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

                memcpy(&c->creds, CMSG_DATA(cmsgp), sizeof(struct ucred));
                printf("Received credentials: pid=%ld, uid=%ld, gid=%ld\n",
                        (long)c->creds.pid, (long)c->creds.uid, (long)c->creds.gid);

                num_clients++;
                printf("server: created client connection %d\n", new_client);
            } else {
                close(new_client);
            }
        }
        // Check existing clients
        for (int i = 1; i < num_clients; i++) {
            printf("inhere");
            Client *c = &clients[i];
            if (c->pollfd.revents & (POLLHUP | POLLERR)) {
                
                printf("server: closing client connection %d\n", c->pollfd.fd);
                close(c->pollfd.fd);
                
                // Replace the disconnected client with the last one in the array
                clients[i] = clients[num_clients - 1];
                num_clients--;
        
                i--;
        
                continue;
            }

            if (c->pollfd.revents & POLLIN) {
            }
        }
    }
}
