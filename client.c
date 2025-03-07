#include "scm_cred_send.h"
#include "scm_cred_recv.h"
int main() {
    int sfd = client_connect();
    
    struct json_object* articles = list_articles(sfd);

    // Print JSON as a string
    printf("JSON Output: %s\n", json_object_to_json_string(articles));


    int ids[] = {1};  
    int num_ids = 1;  

    int* fds = get_articles(sfd, ids, &num_ids);

    if (fds != NULL) {        
        for (int i = 0; i < num_ids; ++i) {
            printf("File descriptor for article %d: %d\n", ids[i], fds[i]);

            // Check if the FD is valid
            if (fds[i] < 0) {
                fprintf(stderr, "Invalid file descriptor for article %d\n", ids[i]);
                continue;
            }

            // Check if FD is open by trying a non-blocking read
            char test_buf[1];
            ssize_t test_read = read(fds[i], test_buf, 1);
            if (test_read == 0) {
                printf("File descriptor %d is empty (EOF reached).\n", fds[i]);
                close(fds[i]);
                continue;
            } else if (test_read < 0) {
                perror("Test read failed");
            } else {
                // Put back the character if successfully read
                lseek(fds[i], -1, SEEK_CUR);
            }

            // Attempt to read using fdopen()
            FILE *file = fdopen(fds[i], "r");
            if (file) {
                printf("Contents of article %d:\n", ids[i]);

                char buffer[1024];
                int count = 0; // Prevent infinite loops
                while (fgets(buffer, sizeof(buffer), file) != NULL) {
                    printf("%s", buffer);
                    fflush(stdout);
                    if (++count > 100) {  // Limit reads to prevent infinite loop
                        printf("\n[WARNING] Too many lines, possible infinite loop.\n");
                        break;
                    }
                }
                fclose(file);
            } else {
                perror("fdopen failed");
            }
        }
        free(fds);
    } else {
        printf("Failed to retrieve articles.\n");
    }
    close_connection(sfd);

}