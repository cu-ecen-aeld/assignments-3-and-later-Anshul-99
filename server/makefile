# Author: Anshul Somani
# This makefile builds an executable for writer.c for two different targets, native and aarch64-none-linux-gnu.
	
#CROSS_COMPILE =
CC=$(CROSS_COMPILE)gcc

CFLAGS=-g -Wall -Werror

all: aesdsocket

default: aesdsocket

aesdsocket: aesdsocket.o
	$(CC) -o aesdsocket aesdsocket.o
	
appl=aesdsocket
	
clean: 
	rm -f aesdsocket aesdsocket.o
	
.PHONY: clean

#valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose --lof-file=/tmp/valgrind-out.txt ./aesdsocket
