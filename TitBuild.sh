#!/bin/bash

#clang++ sph.cpp -std=c++17 -Ofast -march=native -o Tit.out && echo 'compiled.'
g++-10 -Isrc src/sph.cpp -std=c++17 -Ofast -march=native -o Tit.out && echo 'compiled.'
#g++-10 sph.cpp -std=c++17 -O0 -g -o Tit.out && echo 'compiled.'
