#!/bin/sh

# g++ src/main.cpp `pkg-config gtkmm-3.0 --cflags --libs` -std=c++17 -o main

cd `dirname $0`
cmake -S . -B build
cmake --build build
build/voicesync
