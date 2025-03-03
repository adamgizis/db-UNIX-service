# db-UNIX-service

This is polling set up with the passing credientals

gcc scm_cred_recv.c unix_sockets.c error_functions.c -o scm_cred_recv
./scm_cred_recv

gcc scm_cred_send.c unix_sockets.c error_functions.c -o scm_cred_send
./scm_cred_send