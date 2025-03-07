#!/bin/sh

echo "starting"

make all

./server &  

./client_max

pkill server

exit

