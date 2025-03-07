#include "scm_cred_send.h"

int main() {
    int sfd = client_connect();
    // int ids[] = {1, 2};  
    // int num_ids = 2;  


    // int* fds = get_articles(sfd, ids, &num_ids); 
    
    
    // if (fds != NULL) {
    //     for (int i = 0; i < num_ids; ++i) {
    //         printf("File descriptor for article %d: %d\n", ids[i], fds[i]);
    //     }
        
    //     free(fds);
    // } else {
    //     printf("Failed to retrieve articles.\n");
    // }
	// sleep(30);
    upload_article(sfd, "hello.txt", "hello");
    


	close_connection(sfd);
}

