#!/bin/bash

# Check include paths.
./etc/scripts/iwyu.sh src || exit $?

# Normal setup.
CXX=g++-13

# MPI setup (for the future).
#export OMPI_CXX=$CXX
#CXX=mpic++

function compile() {
  $CXX \
    -I./src \
    -I/opt/homebrew/include \
    -L/opt/homebrew/lib \
    -std=c++23 \
    -ltbb -ltbbmalloc \
    -Ofast -march=native -DNDEBUG \
    -o $(echo $1 | sed 's/\.[^.]*$/.out/') \
    $1
}

compile ./src/sph.cpp
