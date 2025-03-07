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


// create a file upload it and then delete it

void upload_file_then_delete() { 
    int sfd = client_connect();

    // CHECK LIST JSON ID SIZE
    struct json_object *json_obj = list_articles(sfd);

    // Get the "articles" array from the JSON object
    int pre_num_articles = 0;
    struct json_object *articles_array = NULL;
    if (json_object_object_get_ex(json_obj, "articles", &articles_array)) {
        // Get the number of articles in the array
        pre_num_articles = json_object_array_length(articles_array);
    }
    json_object_put(json_obj);

    const char *filepath = "test.txt";  // Replace with a valid file path
    const char *title = "test";
    
    // Create the file and write content to it
    int fd = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("open");
        return;
    }

    const char *content = "Hello World TEST";
    ssize_t bytes_written = write(fd, content, strlen(content));
    if (bytes_written < 0) {
        perror("write");
        close(fd);
        return;
    }

    close(fd);  // Close the file after writing

    // Simulate calling the upload_article function
    int result = upload_article(sfd, filepath, title);
    
    if (result == 0) {
        // CHECK LIST JSON ID SIZE
        json_obj = list_articles(sfd);

        // Get the "articles" array from the JSON object
        int post_num_articles = 0;
        if (json_object_object_get_ex(json_obj, "articles", &articles_array)) {
            post_num_articles = json_object_array_length(articles_array);
        }
        json_object_put(json_obj);

        if (pre_num_articles + 1 == post_num_articles) {
            // Loop through each article in the "articles" array
            for (int i = 0; i < post_num_articles; i++) {
                struct json_object *article = json_object_array_get_idx(articles_array, i);

                // Get the "title" from the article
                struct json_object *title_obj = NULL;
                if (json_object_object_get_ex(article, "title", &title_obj)) {
                    const char *article_title = json_object_get_string(title_obj);

                    // Check if the title matches "test"
                    if (strcmp(article_title, "test") == 0) {
                        int id = json_object_get_int(json_object_object_get(article, "id"));

                        // Delete by the id
                        int ids[] = {id};
                        int num_ids = 1;
                        delete_articles(sfd, ids, &num_ids);

                        // Check if the number of articles after deletion matches the pre-upload number
                        json_obj = list_articles(sfd);
                        int del_num_articles = 0;
                        if (json_object_object_get_ex(json_obj, "articles", &articles_array)) {
                            del_num_articles = json_object_array_length(articles_array);
                        }
                        json_object_put(json_obj);

                        if (del_num_articles == pre_num_articles) {
                            // Success
                            printf("SUCCESS");

                        } else {
                            // Failure
                            printf("FAILURE");
                        }
                    }
                }
            }
        }
    }
    close_connection(sfd);
    return;
}

// show unable to delete a file created by root when not root
void delete_file_not_root(){
    int sfd = client_connect();  // Dummy socket file descriptor for simulation

    // Specify the article IDs to delete (we are deleting article with ID 2)


    int ids[] = {0};
    int num_ids = 1;

    // Call the delete_articles function to delete the article with ID 2
    int result = delete_articles(sfd, ids, &num_ids);

    // check if successful
    close_connection(sfd);
}
int main(){
    get_single_file();
    upload_file_then_delete();
    delete_file_not_root();



    
}
