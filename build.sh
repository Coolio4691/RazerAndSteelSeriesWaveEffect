#!/bin/bash
rm ./main

g++ -g -lfmt -ljsoncpp -ljpeg `pkg-config --cflags --libs opencv4` \
    -lX11 -lXrandr -lpython3.10 -ldbus-1 -lpthread \
    `pkg-config --libs --cflags Qt5Core` \
    `pkg-config --libs --cflags Qt5Gui` \
    `pkg-config --libs --cflags Qt5DBus` \
    -Wall -fPIC -o main main.cpp && ./main