# Author: Anshul Somani
# This makefile builds an executable for writer.c for two different targets, native and aarch64-none-linux-gnu.

writer: writer.c
	gcc -Wall -Werror -o writer writer.c 

clean:
	rm -f writer
	
CROSS_COMPILE:
	aarch64-none-linux-gnu-gcc -g -Wall -c -o writer.o writer.c

	
