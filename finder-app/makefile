# Author: Anshul Somani
# This makefile builds an executable for writer.c for two different targets, native and aarch64-none-linux-gnu.
	
CROSS_COMPILE = aarch64-none-linux-gnu-
CC = $(CROSS_COMPILE)gcc

CFLAGS = -g -Wall -Werror

all: writer

default: writer

writer: writer.o
	$(CC) $< -o $@
	
writer.o: writer.c
	$(CC) $(CFLAGS) -c $< -o $@
	
clean: 
	rm -f writer writer.o
	
.PHONY: clean
