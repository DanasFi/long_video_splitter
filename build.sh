#!/bin/bash
gcc -g -o a.out src/main.c `pkg-config --cflags --libs gtk4 libavformat libavcodec libavutil`
