#!/bin/bash
gcc -I./src -g src/video_splitter.c -o a.out src/main.c `pkg-config --cflags --libs gtk4 libavformat libavcodec libavutil`
