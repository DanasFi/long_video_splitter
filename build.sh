#!/bin/bash
gcc `pkg-config --cflags gtk4` -o a.out src/main.c `pkg-config --libs gtk4`
