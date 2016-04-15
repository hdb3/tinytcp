#!/bin/bash -ve
gcc -D_GNU_SOURCE -o bulkclient bulkclient.c
gcc -D_GNU_SOURCE -o bulkserver bulkserver.c
