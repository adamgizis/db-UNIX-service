/* unix_sockets.h

   Header file for unix_sockets.c.
*/
#ifndef UNIX_SOCKETS_H
#define UNIX_SOCKETS_H      /* Prevent accidental double inclusion */

#include <sys/socket.h>
#include <sys/un.h>

int unixBuildAddress(const char *path, struct sockaddr_un *addr);

int unixConnect(const char *path, int type);

int unixBind(const char *path, int type);

#endif