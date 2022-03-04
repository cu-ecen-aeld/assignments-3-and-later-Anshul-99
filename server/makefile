# Author: Anshul Somani
# This makefile builds an executable for writer.c for two different targets, native and aarch64-none-linux-gnu.
	
#CROSS_COMPILE =
CC=$(CROSS_COMPILE)gcc
CFLAGS=-Werror -Wall -g

all: aesdsocket
default: aesdsocket

aesdsocket: aesdsocket.o
	$(CC) -o aesdsocket aesdsocket.o
	
#appl=aesdsocket
	
.PHONY: clean	
clean: 
	rm -f writer writer.o #rm -f $(appl) *.o
	


