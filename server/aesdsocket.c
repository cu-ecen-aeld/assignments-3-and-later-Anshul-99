#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "errno.h"
#include <syslog.h>
#include <fcntl.h>
#include <unistd.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

//#define DOMAIN_SOCKET (PF_INET6)
#define SERVICE_PORT "9000"
#define PATH_TO_FILE "/var/tmp/aesdsocketdata.txt"
#define BYTES 1024

int main()
{
	
	/*Start syslog daemon*/
    openlog("aesdsocket", LOG_USER, LOG_DEBUG|LOG_ERR); 
	

	int errnum =0;
	int socket_fd =0;
	int socket_fd_connected =0;
	int ret_val;
	const char *service_port = SERVICE_PORT;
	
	uint32_t total_message_len = 0;

	
	//Create sockaddr
	//struct sockaddr sock_addr;
	struct addrinfo hints;
	struct addrinfo *servinfo;
	struct sockaddr_storage servinfo_connectingaddr; //to store connecting address from accept()
	socklen_t addr_size;
	
	memset(&hints, 0, sizeof(hints));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	
	ret_val = getaddrinfo(NULL, service_port, &hints, &servinfo);
	if(ret_val != 0)
	{
		errnum = errno;
		syslog(LOG_ERR, "getaddrinfo() returned error. The error was %s\n", strerror(errno));
		
		printf("Error: getaddrinfo() returned error %d\n\r", errnum);
		return -1;
	}
	
	// Get socket fd
	socket_fd = socket(servinfo->ai_family, servinfo->ai_socktype, 0);
	if(socket_fd == -1)
	{
		errnum = errno;
		syslog(LOG_ERR, "Unable to create the socket. The error was %s\n", strerror(errno));
		
		printf("Error: Unable to create socket %d\n\r", errnum);
		return -1;
	}
	
	// bind()
	ret_val = bind(socket_fd, (servinfo->ai_addr), servinfo->ai_addrlen);
	if(ret_val == -1)
	{
		errnum = errno;
		syslog(LOG_ERR, "bind() returned error. The error was %s\n", strerror(errno));
		
		printf("Error: bind() returned error %d\n\r", errnum);
		remove("/var/tmp/aesdsocketdata.txt");
		return -1;
	}
	
	int fd = open(PATH_TO_FILE, O_RDWR | O_CREAT | O_APPEND, 0644);
	
	//TODO: while() check if SIGINT or SIGTERM is received

	int z =3;
	while(z--)
	{
		
		//listen()
		ret_val = listen(socket_fd, 10); /* backlog value might cause problems */
		if(ret_val == -1)
		{
			errnum = errno;
			syslog(LOG_ERR, "listen() returned error. The error was %s\n", strerror(errno));
			
			printf("Error: listen() returned error %d\n\r", errnum);
		}
		
		addr_size = sizeof servinfo_connectingaddr;
		//accept()
		socket_fd_connected = accept(socket_fd, (struct sockaddr *)&servinfo_connectingaddr, &addr_size);
		if(socket_fd_connected == -1)
		{
			errnum = errno;
			syslog(LOG_ERR, "accept() returned error. The error was %s\n", strerror(errno));
			
			printf("Error: accept() returned error %d\n\r", errnum);
		}
		
		//read/write using recv()/send()
	
		
		
		char* incoming_data = (char *)malloc(BYTES*sizeof(char)); /* realloc if buffer is full */
		/* Read , write and re-read again. */
		/* read byte by byte */
		if(incoming_data == NULL)
		{
			printf("Unable to malloc\n\r");
			return -1;
		}
		
		ssize_t msg_length = 0;
		
		uint8_t rx_complete =0;
		
		
		while(1)
		{
			uint32_t index_buffer = 0;
			msg_length = recv(socket_fd_connected, incoming_data, BYTES, MSG_DONTWAIT);
			if(msg_length == -1)
			{
				errnum = errno; //error logging
				
				
				if(errnum == 11)
				{
					printf("Recv() complete\n\r");
					//continue;
					break;
				}
				
				syslog(LOG_ERR, "recv() returned error. The error was %s\n", strerror(errno));
				/* No more messages available */
				printf("No more data is available to read. recv() returned error %d\n\r", errnum);
				//write to file 
				rx_complete = 1; //set complete flag
				
			}
			else
			{
				total_message_len += msg_length;
			}
			
					//check if complete flag is set and exit loop
			if(rx_complete)
			{
				break;
			}
		
			//  write into the file byte by byte
			while(index_buffer<msg_length) 
			{
				if(incoming_data[index_buffer] != '\n') //accordingly write/append to the file
				{
					//write the byte into the file
					ret_val = write(fd, &incoming_data[index_buffer], 1);
					if(ret_val == -1)
					{
						errnum = errno; //error logging			
						syslog(LOG_ERR, "write() returned error. The error was %s\n", strerror(errno));
						
						printf("write() returned error %d\n\r", errnum);
					}
					printf("W: %s\n\r", &incoming_data[index_buffer]);
					
				}
				else if(incoming_data[index_buffer] == '\n')
				{
					
					
					//add new line into the file to separate the packets
					write(fd, &incoming_data[index_buffer], 1);
					if(ret_val == -1)
					{
						errnum = errno; //error logging			
						syslog(LOG_ERR, "write() (append) returned error. The error was %s\n", strerror(errno));
						
						printf("write() (append) returned error %d\n\r", errnum);
					}
					printf("W_A: %s\n\r", &incoming_data[index_buffer]);
				}
				index_buffer++;
			}
			/*close(fd);
			if(ret_val == -1)
			{
				errnum = errno; //error logging			
				syslog(LOG_ERR, "close() returned error. The error was %s\n", strerror(errno));
				
				printf("close() returned error %d\n\r", errnum);
			}*/
		}
		
		char *outgoing_data = (char *)(malloc(total_message_len*sizeof(char)));
		
		//fd = open(PATH_TO_FILE, O_RDWR, 0644);
		lseek(fd, 0, SEEK_SET);
		ret_val = read(fd, outgoing_data, total_message_len);
		if(ret_val == -1)
		{
			errnum = errno; //error logging			
			syslog(LOG_ERR, "read() returned error. The error was %s\n", strerror(errno));
			
			printf("read() returned error %d\n\r", errnum);
		}
		printf("retval: %d\n\r", ret_val);
		printf("T: %d\n\r", total_message_len);
		printf("R: %s", outgoing_data);
		
		//send()
		ret_val = send(socket_fd_connected, outgoing_data, total_message_len, MSG_DONTWAIT);
		if(ret_val == -1)
		{
			errnum = errno; //error logging			
			syslog(LOG_ERR, "send() returned error. The error was %s\n", strerror(errno));
			
			printf("send() returned error %d\n\r", errnum);
		}
		printf("retval: %d\n\r", ret_val);
		
		
		free(incoming_data);
		free(outgoing_data);
		close(socket_fd_connected);
		
		
	}
	
	close(fd);
	if(ret_val == -1)
	{
		errnum = errno; //error logging			
		syslog(LOG_ERR, "close() returned error. The error was %s\n", strerror(errno));
		
		printf("close() returned error %d\n\r", errnum);
	}
	freeaddrinfo(servinfo);
	remove("/var/tmp/aesdsocketdata.txt");
	
	close(socket_fd);
}		
	


	


