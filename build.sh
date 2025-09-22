#!/usr/bin/env sh

if [ ! -d build ]; then
  mkdir build
fi

g++ -c main.cpp -o main.o `pkg-config fuse3 --cflags --libs` -D_FILE_OFFSET_BITS=64
g++ -c ./src/disk.cpp -o disk.o

g++ main.o disk.o -o ./build/simplefs `pkg-config fuse3 --cflags --libs` -D_FILE_OFFSET_BITS=64

rm main.o
rm disk.o