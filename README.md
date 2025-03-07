# db-UNIX-service- Adam Gizis and Laith Najjab

## Description
We have created a Daemon that controls a wikipedia style database that holds .txt file. Each client can connect to the database, and upload, get articles, list all articles, or delete an article. An article can only be deleted from the database by the user who is the author of the article or if the user root.

## Implementation
The server code is in scm_cred_recv.c and the client code is scm_cred_send.c

- Use UNIX domain sockets for communication with a known path for the name of the socket. - Clients and the server connect to a known unix domain socket in unix_sockets.c
- Use accept to create a connection-per-client as in here - The server accepts clients in scm_cred_recv.c in the server() function
- Use event notification (epoll, poll, select, etc...) to manage concurrency as in here- The server uses poll in scm_cred_recv.c to handle event notification.
- Use domain socket facilities to get a trustworthy identity of the client (i.e. user id)- The client uses SCM_CREDENTIALS to pass the pid,uid, and gid. 
- Pass file descriptors between the service and the client- Files are passed from the client to the database to upload and return from the database to the client.


### Install Dependecies
sudo apt update
sudo apt install -y libjson-c-dev sqlite3 libsqlite3-dev

### Running Basic case
make all
./client
./server

## Usage
The client can interact with the server using the following api. 

#import scm_cred_send.h
int client_connect(void); - returns an fd to connect to the database server
int close_connection(int sfd); - closes connection to database server
int* get_articles(int sfd, int* ids, int* num_ids); - Take in an input array of article ids returns a list of file descriptors where the files are open
int delete_articles(int sfd, int* ids, int* num_ids); - Deletes the listed ids from the database if user has proper previleges
int upload_article(int sfd, const char* filepath, const char* title); - Takes in a single filepath and uploads file to the database
struct json_object * list_articles(int sfd); - returns a json_object with the author, id, and title of every article in the database

## Example Test Files



## Work Split
Laith handled the json operations and sending the passing file descriptors both directions. Adam implemented the polling and accept & the client side. A good amount was done in pair.

## Attributions 

Lots of code taken from , The Linux Programming Interface
