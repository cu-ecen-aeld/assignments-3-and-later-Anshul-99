#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "errno.h"
#include <syslog.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>


#define SERVICE_PORT "9000"
#define PATH_TO_FILE "/var/tmp/aesdsocketdata.txt"
#define BYTES 1024

int main()
{
	
	/*Start syslog daemon*/
    openlog("aesdsocket", LOG_USER, LOG_DEBUG|LOG_ERR); 
	int errnum =0;
	int ret_val;

	int socket_fd =0;
	int socket_fd_connected =0;
	const char *service_port = SERVICE_PORT;
	
	int fd;
	
	int total_bytes = 0; 
	char static_buff[16];


/******************************************************

	getaddrinfo()
	
******************************************************/
	
	//Create sockaddr
	struct addrinfo hints;
	struct addrinfo *servinfo;
	struct sockaddr_storage servinfo_connectingaddr; //to store connecting address from accept()
	socklen_t addr_size;
	
	memset(&hints, 0, sizeof(hints));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_INET;  //UNSPEC
	hints.ai_socktype = SOCK_STREAM;
	
	ret_val = getaddrinfo(NULL, service_port, &hints, &servinfo);
	if(ret_val != 0)
	{
		errnum = errno;
		syslog(LOG_ERR, "getaddrinfo() returned error. The error was %s\n", strerror(errnum));
		
		printf("Error: getaddrinfo() returned error %d\n\r", errnum);
		//freeaddrinfo(servinfo);
		return -1;
	}
	
/******************************************************

	socket()
	
******************************************************/
	socket_fd = socket(servinfo->ai_family, servinfo->ai_socktype, 0);
	if(socket_fd == -1)
	{
		errnum = errno;
		syslog(LOG_ERR, "Unable to create the socket. The error was %s\n", strerror(errnum));
		
		printf("Error: Unable to create socket %d\n\r", errnum);
		freeaddrinfo(servinfo);
		return -1;
	}
	
/******************************************************

	bind()
	
******************************************************/
	ret_val = bind(socket_fd, (servinfo->ai_addr), servinfo->ai_addrlen);
	if(ret_val == -1)
	{
		errnum = errno;
		syslog(LOG_ERR, "bind() returned error. The error was %s\n", strerror(errnum));
		
		printf("Error: bind() returned error %d\n\r", errnum);
		freeaddrinfo(servinfo);
		return -1;
	}
	
/******************************************************

	creat() & close()
	
******************************************************/
	fd = creat(PATH_TO_FILE, 0644);
	if(fd == -1)
	{
		errnum = errno;
		syslog(LOG_ERR, "bind() returned error. The error was %s\n", strerror(errnum));
		
		printf("Error: bind() returned error %d\n\r", errnum);
		freeaddrinfo(servinfo);
		return -1;
	}
	
	ret_val = close(fd);
	if(ret_val == -1)
	{
		errnum = errno; //error logging			
		syslog(LOG_ERR, "close() returned error. The error was %s\n", strerror(errnum));
		
		printf("close() returned error %d\n\r", errnum);
		freeaddrinfo(servinfo);
		remove(PATH_TO_FILE);
		return -1;
	}
	
	//freeaddrinfo(servinfo);
	

	
	int z = 4;
	while(z--)	
	{	
		char* incoming_data = (char *)malloc(BYTES*sizeof(char));
		if(incoming_data == NULL)
		{
			printf("malloc failed\n\r");
			freeaddrinfo(servinfo);
			return -1;
		}

		memset(incoming_data, 0, BYTES);
	
/******************************************************

	listen()
	
******************************************************/

		ret_val = listen(socket_fd, 10); /* backlog value might cause problems */
		if(ret_val == -1)
		{
			errnum = errno;
			syslog(LOG_ERR, "listen() returned error. The error was %s\n", strerror(errnum));
			
			printf("Error: listen() returned error %d\n\r", errnum);

			freeaddrinfo(servinfo);
			return -1;
		}
		
		
/******************************************************

	accept()
	
******************************************************/
		addr_size = sizeof servinfo_connectingaddr;
		socket_fd_connected = accept(socket_fd, (struct sockaddr *)&servinfo_connectingaddr, &addr_size);
		if(socket_fd_connected == -1)
		{
			errnum = errno;
			syslog(LOG_ERR, "accept() returned error. The error was %s\n", strerror(errnum));
			
			printf("Error: accept() returned error %d\n\r", errnum);
			
			freeaddrinfo(servinfo);
			return -1;
		}
		
		char ip_addr[INET_ADDRSTRLEN];
		
		//TODO: print the IP address of client
		struct sockaddr_in *addr = (struct sockaddr_in *)&servinfo_connectingaddr;
		
		inet_ntop(AF_INET, &(addr->sin_addr), ip_addr, INET_ADDRSTRLEN);
		syslog(LOG_DEBUG, "Accepted Connection from %s\n\r", ip_addr);
		printf("Accepting connection from %s\n\r", ip_addr);
		
/******************************************************

	recv() bytes and store them in a static buffer 
	temporarily before realloc() it
	
******************************************************/		

		
		ssize_t msg_length = 0;

		int num_bytes = 0;
		 
		//char* outgoing_data = NULL;
		uint8_t completed = 0;

		while(!completed)
		{
			memset(&static_buff[0], 0, 16);
			
			msg_length = recv(socket_fd_connected, &static_buff[0], 16,0); // MSG_WAITALL
			if(msg_length <0)
			{
				errnum = errno;
				syslog(LOG_ERR, "recv() returned error. The error was %s\n", strerror(errnum));
				printf("No more data is available to read. recv() returned error %d\n\r", errnum);
			}
			else if(msg_length == 0)
			{
			//TODO:
				printf("No more messages\n\r");
				//completed = 1;
				break;
			}
			//TODO: can add a condition if msg_length == 16 then directly realloc
			
			for(int i = 0; i<16; i++)
			{
				if(static_buff[i] == 10)
				{
					num_bytes = i+1;
					completed = 1;
					break;
				}
			}
			printf("sB %s ", &static_buff[0]);
			
			total_bytes += num_bytes;
			printf("total_bytes: %d\n\r", total_bytes);
			//printf("num_bytes: %d\n\r",num_bytes);
			
			incoming_data = (char *)realloc(incoming_data, (total_bytes+1));
			if(incoming_data == NULL)
			{
				printf("realloc failed\n\r");
				return -1;
			}	
			
			//printf("data: %s", incoming_data);
			
			strncat(incoming_data, &static_buff[0],num_bytes);	
			//printf("data: %s", incoming_data);	
		}
		//printf("length_recv: %ld\n\r", sizeof(incoming_data));
		printf("recv: %s", incoming_data);
	
/******************************************************

	writing the realloc() buffer into aesdsocketdata.txt
	using write()
	
******************************************************/	

		fd = open(PATH_TO_FILE, O_WRONLY|O_APPEND , 0644);
		if(fd<0)
		{
			errnum = errno; //error logging			
			syslog(LOG_ERR, "open() returned error. The error was %s\n", strerror(errnum));
			
			printf("open() returned error %d\n\r", errnum);
		}
		
		//printf("total_bytes: %d\n\r", total_bytes);
		
		ret_val = write(fd, incoming_data, strlen(incoming_data));
		if(ret_val == -1)
		{
			errnum = errno; //error logging			
			syslog(LOG_ERR, "write() returned error. The error was %s\n", strerror(errnum));
			
			printf("write() returned error %d\n\r", errnum);
			return -1;
		}
		else if(ret_val != strlen(incoming_data))
		{
			syslog(LOG_ERR, "Partial Write\n\r");
			
			printf("partial Write\n\r");
			return -1;
		}
		printf("Written: %d\n\r", ret_val);
		ret_val = close(fd);
		if(ret_val == -1)
		{
			errnum = errno; //error logging			
			syslog(LOG_ERR, "close() returned error. The error was %s\n", strerror(errnum));
			
			printf("close() returned error %d\n\r", errnum);
		}
		
		
/******************************************************

	open() file, read() all the characters from it, 
	then send() it
	
******************************************************/	

		//TODO: read and send bytes by byte
		
		int bytes = total_bytes; //strlen(incoming_data);
		char buff;
		
		printf("bytes: %d\n\r", bytes);

		fd = open(PATH_TO_FILE, O_RDONLY);
		if(fd<0)
		{
			errnum = errno; //error logging			
			syslog(LOG_ERR, "open() returned error. The error was %s\n", strerror(errnum));
			
			printf("open() returned error %d\n\r", errnum);
		}
		
		lseek(fd, 0, SEEK_SET);
		printf("send: ");
		while(bytes--)
		{
			//outgoing_data = (char *)malloc(total_bytes*(sizeof(char)));
			
			
			ret_val = read(fd, &buff, 1);
			if(ret_val == -1)
			{
				errnum = errno; //error logging			
				syslog(LOG_ERR, "read() returned error. The error was %s\n", strerror(errnum));
				
				printf("read() returned error %d\n\r", errnum);
			}
			
			//printf("ret_val_read: %d\n\r", ret_val);
			printf("%s", &buff);
			ret_val = send(socket_fd_connected, &buff, 1, 0);
			if(ret_val == -1)
			{
				errnum = errno; //error logging			
				syslog(LOG_ERR, "send() returned error. The error was %s\n", strerror(errnum));
				
				printf("send() returned error %d\n\r", errnum);
			}	
			//printf("ret_val_rsend: %d\n\r", ret_val);
			
		}
		
		ret_val = close(fd);
		if(ret_val == -1)
		{
			errnum = errno; //error logging			
			syslog(LOG_ERR, "close() returned error. The error was %s\n", strerror(errnum));
			
			printf("close() returned error %d\n\r", errnum);
			freeaddrinfo(servinfo);
			remove(PATH_TO_FILE);
			return -1;
		}
			
		//free(outgoing_data);
		free(incoming_data);
		
		syslog(LOG_DEBUG, "Closed Connection from %s\n\r", ip_addr);
		printf("Closed Connection from %s\n\r", ip_addr);
	}
	
	close(socket_fd_connected);
	freeaddrinfo(servinfo);	
	close(socket_fd);

}
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	/*int fd = open(PATH_TO_FILE, O_RDWR | O_CREAT | O_APPEND, 0644);
	
	//TODO: while() check if SIGINT or SIGTERM is received

	int z =3;
	while(z--)
	{
		
		//listen()
		ret_val = listen(socket_fd, 10); 
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
	
		
		
		char* incoming_data = (char *)malloc(BYTES*sizeof(char)); // realloc if buffer is full 
		// Read , write and re-read again. 
		// read byte by byte 
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
					break;
				}
				
				syslog(LOG_ERR, "recv() returned error. The error was %s\n", strerror(errno));
				// No more messages available 
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
			//while(index_buffer<msg_length) 
			{

				//write the byte into the file
				ret_val = write(fd, incoming_data, msg_length);
				if(ret_val == -1)
				{
					errnum = errno; //error logging			
					syslog(LOG_ERR, "write() returned error. The error was %s\n", strerror(errno));
					
					printf("write() returned error %d\n\r", errnum);
				}
				printf("W: %s\n\r", &incoming_data[index_buffer]);
					
				//index_buffer++;
			}
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
} */		
	


	


