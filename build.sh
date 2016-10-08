#!/bin/bash

cc=gcc
ld=gcc
cflags="-I include -W all -W extra --std=c99"
libs=""

CC() {
	$cc -c $2 -o $1
}

LD() {
	$ld $2 -o $1 $libs
}

if [ ! -d bin ]; then
	mkdir bin
fi

CC bin/http.o http.c
CC bin/example.o example.c

LD bin/example "bin/example.o bin/http.o"
