/* scm_multi.h

   Header file used by scm_multi_send.c and scm_multi_recv.c.
*/
#define _GNU_SOURCE             /* To get SCM_CREDENTIALS definition from
                                   <sys/socket.h> */
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>
#include "unix_sockets.h"       /* Declares our socket functions */
#include "tlpi_hdr.h"

#define SOCK_PATH "scm_multi"
#define MAX_FDS 1024            /* Maximum number of file descriptors we'll attempt to exchange in ancillary data */