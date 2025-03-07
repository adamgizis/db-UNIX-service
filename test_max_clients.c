#include "scm_cred_send.h"

int main() {

    int pid;
    for(int i = 0; i < MAX_FDS - 1; i++){   
        pid = fork();
        if(!pid){
            char* argument_list[] = {"./client", NULL};
            if(execvp(argument_list[0], argument_list) < 0){
                printf("I'm erroring\n");
            }
        }   

    }

    printf("parent reached end of loop\n");

    int sfd = client_connect();

    sleep(5);  

    if (fcntl(sfd, F_GETFD) > 0) {
        printf("TEST FAILED\n");
    }
    else {
        printf("TEST SUCCESS\n");   
    }

}