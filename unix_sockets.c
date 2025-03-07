/* unix_sockets.c

   A package of useful routines for UNIX domain sockets.
*/
#include "unix_sockets.h"       /* Declares functions defined here */
#include "tlpi_hdr.h"
/* Build a UNIX domain socket address structure for 'path', returning
   it in 'addr'. Returns -1 on success, or 0 on error. */

int
unixBuildAddress(const char *path, struct sockaddr_un *addr)
{
    if (addr == NULL || path == NULL) {
        errno = EINVAL;
        return -1;
    }

    memset(addr, 0, sizeof(struct sockaddr_un));
    addr->sun_family = AF_UNIX;
    if (strlen(path) < sizeof(addr->sun_path)) {
        strncpy(addr->sun_path, path, sizeof(addr->sun_path) - 1);
        return 0;
    } else {
        errno = ENAMETOOLONG;
        return -1;
    }
}
/* Create a UNIX domain socket of type 'type' and connect it
   to the remote address specified by the 'path'.
   Return the socket descriptor on success, or -1 on error */

int
unixConnect(const char *path, int type)
{
    struct sockaddr_un addr;

    if (unixBuildAddress(path, &addr) == -1)
        return -1;

    int sd = socket(AF_UNIX, type, 0);
    if (sd == -1)
        return -1;

    if (connect(sd, (struct sockaddr *) &addr,
                sizeof(struct sockaddr_un)) == -1) {
        int savedErrno = errno;
        close(sd);                      /* Might change 'errno' */
        errno = savedErrno;
        return -1;
    }

    return sd;
}
/* Create a UNIX domain socket and bind it to 'path'.
   Return the socket descriptor on success, or -1 on error. */

int
unixBind(const char *path, int type)
{
    struct sockaddr_un addr;

    if (unixBuildAddress(path, &addr) == -1)
        return -1;

    int sd = socket(AF_UNIX, type, 0);
    if (sd == -1)
        return -1;

    if (bind(sd, (struct sockaddr *) &addr, sizeof(struct sockaddr_un)) == -1) {
        int savedErrno = errno;
        close(sd);                      /* Might change 'errno' */
        errno = savedErrno;
        return -1;
    }

    return sd;
}