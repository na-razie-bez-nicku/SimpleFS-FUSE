#!/usr/bin/env sh

sudo fusermount3 -u /mnt/simplefs
sudo valgrind --tool=memcheck ./build/simplefs -o disk=/home/ubuntu/projects/SimpleFS-FUSE/disk2.img -o exec,allow_other,default_permissions /mnt/simplefs -f