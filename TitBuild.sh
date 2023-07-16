#!/bin/bash
function compile() {
  g++-13 \
    -I./src \
    -I/opt/homebrew/include \
    -std=c++23 \
    -Ofast -march=native -DNDEBUG \
    -o $(echo $1 | sed 's/\.[^.]*$/.out/') \
    $1
}

compile ./src/sph.cpp
