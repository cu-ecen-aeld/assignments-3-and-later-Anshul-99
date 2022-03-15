/*****************************************************************************
​ * ​ ​ Copyright​ ​ (C)​ ​ 2021 ​ by​ ​ Anshul Somani
​ *
​ * ​ ​ Redistribution,​ ​ modification​ ​ or​ ​ use​ ​ of​ ​ this​ ​ software​ ​ in​ ​ source​ ​ or​ ​ binary
​ * ​ ​ forms​ ​ is​ ​ permitted​ ​ as​ ​ long​ ​ as​ ​ the​ ​ files​ ​ maintain​ ​ this​ ​ copyright.​ ​ Users​ ​ are
​ * ​ ​ permitted​ ​ to​ ​ modify​ ​ this​ ​ and​ ​ use​ ​ it​ ​ to​ ​ learn​ ​ about​ ​ the​ ​ field​ ​ of​ ​ embedded
​ * ​ ​ software.​ ​ Anshul Somani​ and​ ​ the​ ​ University​ ​ of​ ​ Colorado​ ​ are​ ​ not​ ​ liable​ ​ for
​ * ​ ​ any​ ​ misuse​ ​ of​ ​ this​ ​ material.
​ *
*****************************************************************************/
/**
​ * ​ ​ @file​ ​ writer.c
​ *
​ * ​ ​ This file takes a string and path to a file from the command line 
 *	 and then writes that string to given file.
​ *
​ * ​ ​ @author​ ​ Anshul Somani
​ * ​ ​ @date​ ​ Jan 20 2022
​ * ​ ​ @version​ ​ 1.0
​ *
​ */

/* Resources: 1. man pages for syslog(), open(), write() and close().
			  2. https://linux.die.net/man/3/syslog
			  3. https://web.eecs.umich.edu/~sugih/pointers/make.html
			  4. I also took some help from Ananth Deshpande in debugging some errors caused wrong implementation of syslog() and open(). */


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <syslog.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define PATH_ARGUMENT 1
#define STRING_ARGUMENT 2


int main( int argc, char* argv[])
{
	/*Start syslog daemon*/
    openlog("writer", LOG_USER, LOG_DEBUG|LOG_ERR); 
    
    /* Variable to store errno*/
    int errnum;
    
    /*Check if the right number of arguments were received*/
	if(argc != 3)
	{
		/* Display error messages on command line*/
		printf("Error: Incorrect number of arguments\n\r");
		printf(" <executable> <path to file> <string to be written>\n\r");
		
		/* write error and debug messages in syslog*/
		syslog(LOG_ERR, "Incorrect number of arguments.");
		syslog(LOG_DEBUG, "<executable> <path to file> <string to be written>");
		return 1;
	}
	
	/*Open the file*/
	int fd = open(argv[PATH_ARGUMENT], O_RDWR | O_CREAT | O_TRUNC, 0644);
	if(fd == -1)
	{
		/* write error messages in syslog*/
	    syslog(LOG_ERR, "Unable to opening the file. File descriptor value from open() was -1. The error was %s\n", strerror(errno));
	    
	    /* Display error messages on command line*/
		errnum =errno;
	    printf("Error: Unable to opening the file\n\r");
	    printf("ERRNO: %d \n\r",errnum);
	    return 1;
	}
	
	/* write debug messages in syslog*/
	syslog(LOG_DEBUG, "File Opened.");
	
	int str_len = strlen(argv[STRING_ARGUMENT]);
	
	/* Write string to the file*/
	if(str_len != write(fd, argv[STRING_ARGUMENT], str_len))
	{
		/* write error messages in syslog*/
        syslog(LOG_ERR, "Unable to write complete string to file. The error was %s\n", strerror(errno));
        
        /* Display error messages on command line*/
		errnum = errno;
        printf("Error: Unable to write complete string to file\n\r");
        printf("ERRNO: %d \n\r",errnum);
        return 1;
    }
    
    /* write debug messages in syslog*/
	syslog(LOG_DEBUG, "%s %s %s %s", "writing ", argv[STRING_ARGUMENT], " to ", argv[PATH_ARGUMENT]);
    syslog(LOG_DEBUG, "File written to.");
    
    /*Close file*/
    if(close(fd) ==-1)
    {				
    	/* write error messages in syslog*/		
        syslog(LOG_ERR, "Unable to close the file. The error was %s\n", strerror(errno));
        
        /* Display error messages on command line*/
    	errnum = errno;
        printf("Error: Unable to close the file\n\r");
        printf("ERRNO: %d \n\r",errnum);
        return 1;
    } 
    
    /* write debug messages in syslog*/
    syslog(LOG_DEBUG, "File closed");
    return 0;
	
	
}
