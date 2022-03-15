#include "systemcalls.h"

#include "errno.h"
#include <stdlib.h>
#include <syslog.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>

/**
 * @param cmd the command to execute with system()
 * @return true if the commands in ... with arguments @param arguments were executed 
 *   successfully using the system() call, false if an error occurred, 
 *   either in invocation of the system() command, or if a non-zero return 
 *   value was returned by the command issued in @param.
*/
bool do_system(const char *cmd)
{

/*
 * TODO  add your code here
 *  Call the system() function with the command set in the cmd
 *   and return a boolean true if the system() call completed with success 
 *   or false() if it returned a failure
*/

    /*Start syslog daemon*/
    openlog("do_system", LOG_USER, LOG_DEBUG|LOG_ERR); 
	int ret_val;
	int errnum;
	ret_val = system(cmd);
	syslog(LOG_DEBUG, "Calling syslog() from do_system()\n\r");
	
	errnum = errno;
	/* is cmd is NULL then it returns 1 and on successfull commands it returns 0. In case of an error, it'll return -1 
		https://man7.org/linux/man-pages/man3/system.3.html
		https://stackoverflow.com/questions/8654089/return-value-of-system-in-c */
	if(ret_val <0) 
	{
		printf("Error on calling system()");
		printf("ERRNO: %d \n\r",errnum);
		
		syslog(LOG_ERR, "Error on calling system(). The error was %s\n", strerror(errnum));
		return false;
	}

    return true;
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the 
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];

/*
 * TODO:
 *   Execute a system command by calling fork, execv(),
 *   and wait instead of system (see LSP page 161).
 *   Use the command[0] as the full path to the command to execute
 *   (first argument to execv), and use the remaining arguments
 *   as second argument to the execv() command.
 *   
*/


	char* str1 = NULL;
	str1 = command[count-1];

	

	
	/*Start syslog daemon*/
    openlog("do_exec", LOG_USER, LOG_DEBUG|LOG_ERR); 
    
	int errnum;
	int ret;
	int status;
	
	pid_t pid = fork();	
	errnum = errno;
	syslog(LOG_DEBUG, "Calling fork() from do_exec()\n\r");
	
	
	if(pid == -1)
	{
		printf("Error on calling fork()");
		printf("ERRNO: %d \n\r",errnum);
		
		syslog(LOG_ERR, "Error on calling fork(). The error was %s\n", strerror(errnum));
		return false;
	}
	else if(pid)
	{
		printf("Child created with pid %d \n\r", pid);
		syslog(LOG_DEBUG, "Child created\n\r");
		
		syslog(LOG_DEBUG, "Calling waitpid() from do_exec()\n\r");
		
		if(waitpid(pid, &status, 0) == -1)
		{
			errnum = errno;
			syslog(LOG_ERR, "Error on calling waitpid(). The error was %s\n", strerror(errnum));
			return false;
		}
		else if(WIFEXITED(status))
		{
			if(((strncmp(str1, "/", 1)) !=0))
			{
				return false;
			}
			return true;
		}
	}
	
	else if(!pid)
	{
		printf("This is the child process \n\r");
		ret = execv(command[0], command);
		errnum = errno;
		if(((strncmp(str1, "/", 1)) !=0))
		{
			printf("No absolute path\n\r");
			return false;
		}
		
		syslog(LOG_DEBUG, "Calling execv() from do_exec()\n\r");
		if(ret <0)
		{
			printf("Error on calling execv()");
			printf("ERRNO: %d \n\r",errnum);
		
			syslog(LOG_ERR, "Error on calling execv(). The error was %s\n", strerror(errnum));
			return false;
		}
		
		exit(-1);
	}
	syslog(LOG_DEBUG, "Exiting do_exec()\n\r");
	
	
    va_end(args);

    return true;
}

/**
* @param outputfile - The full path to the file to write with command output.  
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];


/*
 * TODO
 *   Call execv, but first using https://stackoverflow.com/a/13784315/1446624 as a refernce,
 *   redirect standard out to a file specified by outputfile.
 *   The rest of the behaviour is same as do_exec()
 *   
*/
	
	/*Start syslog daemon*/
		openlog("do_exec_redirect", LOG_USER, LOG_DEBUG|LOG_ERR); 
		int errnum;
		int fd = open(outputfile, O_WRONLY|O_CREAT|O_TRUNC, 0644);
		if(fd<0)
		{
			errnum = errno;
			/* write error messages in syslog*/
			syslog(LOG_ERR, "Unable to opening the file. The error was %s\n", strerror(errnum));
			
			/* Display error messages on command line*/
			printf("Error: Unable to opening the file\n\r");
			printf("ERRNO: %d \n\r",errnum);
			return false;
		}

		int ret;
		int status;
		pid_t pid = fork();	
		errnum = errno;
		syslog(LOG_DEBUG, "Calling fork() from do_exec()\n\r");
		if(pid == -1)
		{
			printf("Error on calling fork()");
			printf("ERRNO: %d \n\r",errnum);
			
			syslog(LOG_ERR, "Error on calling fork(). The error was %s\n", strerror(errnum));
			return false;
		}

		else if(pid)
		{
			syslog(LOG_DEBUG, "Child created\n\r");

		close(fd);
		
		syslog(LOG_DEBUG, "Calling waitpid() from do_exec()\n\r");
		if(waitpid(pid, &status, 0) == -1)
		{
			errnum = errno;
			syslog(LOG_ERR, "Error on calling waitpid(). The error was %s\n", strerror(errnum));
			return false;
		}
		else if(WIFEXITED(status))
		{
			return true;
		}
		}
		else if(!pid)
		{
			if(dup2(fd, 1)<0)
			{
				printf("Error in dup2\n\r");
				return false;
			}
			close(fd);
			
			ret = execvp(command[0], command);
			errnum = errno;

			syslog(LOG_DEBUG, "Calling execv() from do_exec()\n\r");
			if(ret == -1)
			{
				printf("Error on calling execv()");
				printf("ERRNO: %d \n\r",errnum);
			
				syslog(LOG_ERR, "Error on calling execv(). The error was %s\n", strerror(errnum));
				return false;
			}
			exit(-1);
		}
		
		syslog(LOG_DEBUG, "Exiting do_exec()\n\r");
	
    va_end(args);
    
    return true;
}
