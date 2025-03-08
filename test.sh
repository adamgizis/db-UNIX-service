#!/bin/sh

echo "starting"

make all

sqlite3 database/wiki.db < database/create.sql

./server
