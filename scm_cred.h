/* scm_cred.h

   Header file used by scm_cred_send.c and scm_cred_recv.c.
*/
#define _GNU_SOURCE             /* To get SCM_CREDENTIALS definition from
                                   <sys/socket.h> */
#include <sys/socket.h>
#include <sys/un.h>
#include <stdbool.h>
#include "unix_sockets.h"       /* Declares our socket functions */
#include "tlpi_hdr.h"
#include <sys/poll.h>
#include <grp.h>
#include <json-c/json.h>
#include <sys/stat.h>      
#include <sqlite3.h>
#include <pwd.h>
#include <sys/sendfile.h>
#include <fcntl.h>

#define SOCK_PATH "scm_cred"
#define DB_PATH "database/wiki.db"
#define WIKI_PATH "wiki_content/"

#define BUFFER_SIZE 1024
#define MAX_FDS 16