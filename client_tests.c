#include "scm_cred.h"
#include "scm_client.c"

void 
client_GETARTICLES(){

        struct msghdr msgh;
        prepare_credentials_msg(&msgh);
        if(!msgh){
            errExit("Error Getting msg");
        }        
        /* Connect to the peer socket */

        int sfd;
        sfd = unixConnect(SOCK_PATH, SOCK_STREAM);
        while(sfd < 0){
            sfd = unixConnect(SOCK_PATH, SOCK_STREAM);
        }

        /* Send real plus ancillary data */
        ssize_t ns = sendmsg(sfd, &msgh, 0);
        if (ns == -1)
            errExit("sendmsg");

        printf("sendmsg() returned %zd\n", ns);

        /* Only send credentials once   */
        msgh.msg_control = NULL;
        msgh.msg_controllen = 0;
        
        
        
        struct json_object *json_obj = json_object_new_object();
        json_object_object_add(json_obj, "action", json_object_new_string("GET_ARTICLE"));

        json_object *ids = json_object_new_array();
        json_object_array_add(ids, json_object_new_int(1)); 
        json_object_array_add(ids, json_object_new_int(2));                             
        json_object_object_add(json_obj, "ids", ids);

        // Receive the file descriptors
        int num_fds;
    
        int *fds = receive_fds(sfd, &num_fds);
    
        printf("verifying fd\n");
        //Verify the received FD
        if (fds) {
            printf("Received %d file descriptors:\n", num_fds);
            for (int i = 0; i < num_fds; i++) {
                printf("Recieved %d\n", fds[i]);
                ssize_t bytes_read;
                char buffer[1024];
                    
                printf("printing results\n");   
                while((bytes_read = read(fds[i], buffer, sizeof(buffer))) > 0) {
                    write(STDOUT_FILENO, buffer, bytes_read);
                }
            }
            free(fds);
        } else {
            printf("Failed to receive file descriptors\n");
            
        }
        sleep(30);
    }