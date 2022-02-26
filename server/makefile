# Author: Anshul Somani
# This makefile builds an executable for writer.c for two different targets, native and aarch64-none-linux-gnu.
	

ifeq ($(CC),)
	CC=$(CROSS_COMPILE)gcc	
endif
ifeq ($(CFLAGS),)
	CFLAGS=-Werror -Wall -g  	
endif
ifeq ($(LDFLAGS),)
	LDFLAGS=-pthread	
endif



all: aesdsocket
default: aesdsocket

aesdsocket: aesdsocket.o
	$(CC) $(CFLAGS) $^ -o $@ $(INCLUDES) $(LDFLAGS) #${CC} ${CFLAGS} -o aesdsocket aesdsocket.o ${LDFLAGS} 

#appl=aesdsocket
	
.PHONY: clean	
clean: 
	 rm -f aesdsocket aesdsocket.o #rm -f $(appl) *.o #
	


