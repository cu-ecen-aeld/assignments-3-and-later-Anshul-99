# Author: Anshul Somani
# This makefile builds an executable for writer.c for two different targets, native and aarch64-none-linux-gnu.
	
#CROSS_COMPILE =
CC=$(CROSS_COMPILE)gcc
CFLAGS=-Werror -Wall -g  #adding pthread here gives an error stating undefined reference to pthread_create() and pthread_join() in aesdsocket.c

all: aesdsocket
default: aesdsocket

aesdsocket: aesdsocket.o
	$(CC) -pthread -o aesdsocket aesdsocket.o
	
#appl=aesdsocket
	
.PHONY: clean	
clean: 
	 rm -f aesdsocket aesdsocket.o #rm -f $(appl) *.o #
	


