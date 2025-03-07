#include<scm_cred_send.h>
#include<scm_cred_recv.h>

int main() {

    int pid;
    for(int i = 0; i < MAX_FDS; i++){
        pid = fork();
        if(!pid){
            execvp("./client", NULL);
        }
    }

    int sfd = client_connect();

    char buffer[] = "this won't reach";

    if (write(sfd, buffer, sizeof(buffer)) > 0) {
        printf("TEST FAILED\n");
    }
    else {
        pritnf("TEST SUCCESS\n");   
    }

}