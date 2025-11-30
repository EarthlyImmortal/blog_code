#!/bin/sh
mkdir -p build && cd build
cmake .. -DCMAKE_C_COMPILER=/usr/local/gcc-11.2.0/bin/gcc -DCMAKE_CXX_COMPILER=/usr/local/gcc-11.2.0/bin/g++
make -j$(nproc)