#!/bin/bash

# git submodule update --init --recursive
# git submodule update --remote
cmake -B build/ -D CMAKE_CXX_COMPILER=clang++ .
