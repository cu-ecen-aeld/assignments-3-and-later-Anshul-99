# Author: Anshul Somani
# This makefile builds an executable for writer.c for two different targets, native and aarch64-none-linux-gnu.
	
CROSS_COMPILE =
CC = $(CROSS_COMPILE)gcc

CFLAGS = -g -Wall -Werror

all: aesdsocket

default: aesdsocket

writer: aesdsocket.o
	$(CC) $< -o $@
	
writer.o: aesdsocket.c
	$(CC) $(CFLAGS) -c $< -o $@
	
clean: 
	rm -f aesdsocket aesdsocket.o
	
.PHONY: clean
