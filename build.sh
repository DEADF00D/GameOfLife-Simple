#!/bin/sh
gcc main.c `sdl-config --libs --cflags` -lSDL_ttf -o TheGameOfLife
