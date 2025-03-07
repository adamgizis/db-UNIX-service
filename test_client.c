#include "scm_cred_send.h"
#include "scm_cred_recv.h"


void get_single_file(){
    int sfd = client_connect();

    int ids[] = {1};  
    int num_ids = 1;  

    int* fds = get_articles(sfd, ids, &num_ids);

    if (fds != NULL) {        
        for (int i = 0; i < num_ids; ++i) {
            // Check if the FD is valid
            if (fds[i] < 0) {
                printf("FAILURE\n");
                continue;
            }

            // Check if FD is open by trying a non-blocking read
            char test_buf[1];
            ssize_t test_read = read(fds[i], test_buf, 1);
            if (test_read == 0) {
                close(fds[i]);
                printf("FAILURE\n");
                continue;
            } else if (test_read < 0) {
                printf("FAILURE\n");
            } else {
                // Put back the character if successfully read
                lseek(fds[i], -1, SEEK_CUR);
            }

            // Attempt to read using fdopen()
            FILE *file = fdopen(fds[i], "r");
            if (file) {
                // Try to read and print PASS if successful
                char buffer[1024];
                if (fgets(buffer, sizeof(buffer), file) != NULL) {
                    printf("SINGLE FILE PASS\n");
                } else {
                    printf("FAILURE\n");
                }
                fclose(file);
            } else {
                printf("FAILURE\n");
            }
        }
        free(fds);
    } else {
        printf("FAILURE\n");
    }
    close_connection(sfd);

    return;
}

void upload_file(){ 
 


    int sfd = client_connect();

    // CHECK LIST JSON ID SIZE



    const char *filepath = "file.txt";  // Replace with a valid file path
    const char *title = "Sample Article Title";
    
    // Create the file and write "Hello World" to it
    int fd = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("open");
        return -1;
    }

    const char *content = "Hello World";
    ssize_t bytes_written = write(fd, content, strlen(content));
    if (bytes_written < 0) {
        perror("write");
        close(fd);
        return -1;
    }

    close(fd);  // Close the file after writing

    // Simulate a connection to the server (fake socket fd for simulation)
    int sfd = 12345;  // Dummy socket file descriptor for simulation
    
    // Simulate calling the upload_article function
    int result = upload_article(sfd, filepath, title);
    
    if (result == 0) {
            // CHECK LIST JSON ID SIZE
            
            return 0;
    } else {
        printf("FAIL\n");
    }

    return 0;
}

void delete_file(){
    int sfd = client_connect();  // Dummy socket file descriptor for simulation

    // Specify the article IDs to delete (we are deleting article with ID 2)
    int ids[] = {2};
    int num_ids = 1;

    // Call the delete_articles function to delete the article with ID 2
    int result = delete_articles(sfd, ids, &num_ids);

    // check if successful


}

