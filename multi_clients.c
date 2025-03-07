#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    int num_clients = 5;
    
    for (int i = 0; i < num_clients; i++) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("fork failed");
            exit(1);
        } else if (pid == 0) {
            // Child process: Execute client_exec
            execlp("./client", "client_exec", (char*) NULL);
            perror("execlp failed"); // Only reached if exec fails
            exit(1);
        }
    }

    // Parent process waits for all children
    for (int i = 0; i < num_clients; i++) {
        wait(NULL);
    }

    return 0;
}
