#!/usr/bin/env sh

if [ ! -d build ]; then
  mkdir build
fi

g++ -c -g -O0 main.cpp -o main.o -g `pkg-config fuse3 --cflags --libs` -D_FILE_OFFSET_BITS=64
g++ -c -g -O0 ./src/disk.cpp -o disk.o -g

g++ -g main.o disk.o -o ./build/simplefs `pkg-config fuse3 --cflags --libs` -D_FILE_OFFSET_BITS=64

rm main.o
rm disk.o