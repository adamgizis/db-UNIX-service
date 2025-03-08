#!/bin/sh

echo "TESTING...\n"

make clean

make all

sqlite3 database/wiki.db < database/create.sql

./server &  

./client_test

pkill server

exit

