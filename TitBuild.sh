#!/bin/bash

g++-13 -Isrc src/sph.cpp -std=c++23 -Ofast -march=native -o Tit.out && echo 'compiled.'
