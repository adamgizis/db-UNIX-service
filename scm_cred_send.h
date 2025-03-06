#ifndef CLIENT_H
#define CLIENT_H

#include "scm_cred.h"
// Function Prototypes
int close_connection(int sfd);
int client_connect(void);
void prepare_credentials_msg(struct msghdr *msgh);
int send_files(int sfd, int* fdList, int fdCnt, const char* json_string);
int* receive_fds(int socket, int *num_fds);
int* get_articles(int sfd, int* ids, int* num_ids);
int delete_articles(int sfd, int* ids, int* num_ids);

#endif /* CLIENT_H */