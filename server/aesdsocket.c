#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "errno.h"
#include <syslog.h>

#include <stdlib.h>

//#define DOMAIN_SOCKET (PF_INET6)
#define SERVICE_PORT "9000"
#define PATH_TO_FILE "/var/tmp/aesdsocketdata.txt"
#define BYTES 512

int main()
{

	//TODO: /*makefile */
	
	/*Start syslog daemon*/
    openlog("writer", LOG_USER, LOG_DEBUG|LOG_ERR); 
	

	int errnum =0;
	int socket_fd =0;
	int socket_fd_connected =0;
	int ret_val;
	const char *service_port = SERVICE_PORT;
	
	// Get socket fd
	socket_fd = socket(PF_INET6, SOCK_STREAM, 0);
	if(socket_fd == -1)
	{
		errnum = errno;
		syslog(LOG_ERR, "Unable to create the socket. The error was %s\n", strerror(errno));
		
		printf("Error: Unable to create socket %d\n\r", errnum);
		return -1;
	}
	
	//Create sockaddr
	//struct sockaddr sock_addr;
	struct addrinfo hints;
	struct addrinfo *servinfo;
	struct addrinfo *servinfo_connectingaddr;
	
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
	
	servinfo->sa_family = AF_INET6
	//TODO:
	servinfo->sa_data; //go through beej guide section 3.3
	
	ret_val = getaddrinfo(NULL, service_port, &hints, &servinfo_connectingaddr);
	if(ret_val != 0)
	{
		errnum = errno;
		syslog(LOG_ERR, "getaddrinfo() returned error. The error was %s\n", strerror(errno));
		
		printf("Error: getaddrinfo() returned error %d\n\r", errnum);
		return -1;
	}
	
	// bind()
	ret_val = bind(socket_fd, servinfo, sizeof(struct sockaddr));
	if(ret_val == -1)
	{
		errnum = errno;
		syslog(LOG_ERR, "bind() returned error. The error was %s\n", strerror(errno));
		
		printf("Error: bind() returned error %d\n\r", errnum);
	}
	
	//TODO: while() check if SIGINT or SIGTERM is received
	
	//listen()
	ret_val = listen(sock_fd, 10); /* backlog value might cause problems */
	if(ret_val == -1)
	{
		errnum = errno;
		syslog(LOG_ERR, "listen() returned error. The error was %s\n", strerror(errno));
		
		printf("Error: listen() returned error %d\n\r", errnum);
	}
	
	
	//accept()
	socket_fd_connected = accept(sock_fd, servinfo_connecting_addr, sizeof(struct sockaddr));
	if(socket_fd_connected == -1)
	{
		errnum = errno;
		syslog(LOG_ERR, "accept() returned error. The error was %s\n", strerror(errno));
		
		printf("Error: accept() returned error %d\n\r", errnum);
	}	
	
	//read/write using recv()/send()

	//TODO:
	char* incoming_data = (char *)malloc(BYTES*sizeof(char)); /* realloc if buffer is full */
	/* Read , write and re-read again. */
	/* read byte by byte */
	if(incoming_data == NULL)
	{
		printf("Unable to malloc\n\r");
		return -1;
	}
	
	ssize_t msg_length = 0;
	uint32_t index_buffer = 0;
	uint8_t rx_complete =0;
	int fd;
	uint32_t total_message_len = 0;
	
	while(1)
	{
		msg_length = recv(socket_fd_connected, incoming_data, BYTES, MSG_DONTWAIT);
		if(msg_length <=0)
		{
			errnum = errno; //error logging
			
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
	
		//  write into the file byte by byte
		while(index_buffer<msg_length) 
		{
			if(incoming_data[index_buffer] != '\n') //accordingly write/append to the file
			{
				fd = open(PATH_TO_FILE, O_RDWR | O_CREAT /*| O_TRUNC*/, 0644);
				//write the byte into the file
				ret_val = write(fd, &incoming_data[index_buffer], 1);
				if(ret_val == -1)
				{
					errnum = errno; //error logging			
					syslog(LOG_ERR, "write() returned error. The error was %s\n", strerror(errno));
					
					printf("write() returned error %d\n\r", errnum);
				}
				
			}
			else if(incoming_data[index_buffer] == '\n')
			{
				fd = open(PATH_TO_FILE, O_RDWR | O_CREAT |O_APPEND/*| O_TRUNC*/, 0644); //append the byte into the file
				
				//add new line into the file to separate the packets
				write(fd, &incoming_data[index_buffer], 1);
				if(ret_val == -1)
				{
					errnum = errno; //error logging			
					syslog(LOG_ERR, "write() (append) returned error. The error was %s\n", strerror(errno));
					
					printf("write() (append) returned error %d\n\r", errnum);
				}
			}
			index_buffer++;
		}
		close(fd);
		if(ret_val == -1)
		{
			errnum = errno; //error logging			
			syslog(LOG_ERR, "close() returned error. The error was %s\n", strerror(errno));
			
			printf("close() returned error %d\n\r", errnum);
		}


		//check if complete flag is set and exit loop
		if(rx_complete)
		{
			break;
		}
	}
	
	uint8_t * outgoing_data = (char *)malloc(total_message_len*sizeof(char));
	
	fd = open(PATH_TO_FILE, O_RDWR | O_CREAT /*| O_TRUNC*/, 0644);
	read(fd, outgoing_data, total_message_len);
	
	//send()
	send(socket_fd_connected, outgoing_data, total_message_len, MSG_DONTWAIT);
	
	free(incoming_data);
	free(outgoing_data);
	
}


