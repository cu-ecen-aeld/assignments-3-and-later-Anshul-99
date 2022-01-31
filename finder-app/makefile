# Author: Anshul Somani
# This makefile builds an executable for writer.c for two different targets, native and aarch64-none-linux-gnu.

#writer: writer.c
	#gcc -Wall -Werror -o writer writer.c 

#clean:
	#rm -f writer
	
#CROSS_COMPILE:
	#aarch64-none-linux-gnu-gcc -g -Wall -c -o writer.o writer.c

	
CROSS_COMPILE = 
CC = $(CROSS_COMPILE)gcc

CFLAGS = -g -Wall -Werror

default: writer

writer: writer.o
	$(CC) $< -o $@
	
writer.o: writer.c
	$(CC) $(CFLAGS) -c $< -o $@
	
clean: 
	rm -f writer writer.o

.PHONY: clean
