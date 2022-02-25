#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "errno.h"
#include <syslog.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <pthread.h>
#include <sys/queue.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>


#define SERVICE_PORT "9000"
#define PATH_TO_FILE "/var/tmp/aesdsocketdata.txt"
#define BYTES 128

int socket_fd =0;
int socket_fd_connected =0;

//int fd;

struct addrinfo hints;
struct addrinfo *servinfo;
struct sockaddr_storage servinfo_connectingaddr; //to store connecting address from accept()
socklen_t addr_size;

const char *service_port = SERVICE_PORT;

char ip_addr[INET_ADDRSTRLEN];

int errnum =0;
int ret_val;
int return_val = 0;

int total_bytes = 0;

//char* incoming_data;

typedef struct thread_info thread_info_t;
struct thread_info
{
//TODO: add client data to the struct
	pthread_t thread_id;
	int terminate_thread_flag;
	SLIST_ENTRY(thread_info) entries;
	int sockfd_connected;
};

pthread_mutex_t mutex_aesdsocketdata_file = PTHREAD_MUTEX_INITIALIZER;

//TODO: define a function that handles the cleaning up and is called when returning due to error

static void sig_handler (int signo)
{
	if((signo == SIGINT) || (signo == SIGTERM) || (signo == SIGKILL))
	{
		//freeaddrinfo(servinfo);
		unlink(PATH_TO_FILE);
		//free(incoming_data);
		close(socket_fd_connected);
		close(socket_fd);
	}
}

int setup_comm()
{
/******************************************************

	getaddrinfo()
	
******************************************************/
	
	//Create sockaddr

	
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

	setsockopt()
	
******************************************************/

	//int optval = 1;
	//int optlen = sizeof(optval);

	ret_val = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)); //&optval, optlen
	if(ret_val == -1)
	{
		errnum = errno;
		syslog(LOG_ERR, "setsockopt() returned error. The error was %s\n", strerror(errnum));
		
		printf("Error: setsockopt() returned error %d\n\r", errnum);
		close(socket_fd);
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
	int fd = creat(PATH_TO_FILE, 0644);
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
		
		
		printf("close() returned error %d\n\r", errnum);
		freeaddrinfo(servinfo);
		remove(PATH_TO_FILE);
		return -1;
	}
	
	freeaddrinfo(servinfo);
	return 0;
}

int check_for_connection()
{

		return 0;
}


void * thread_func(void *thread_data)
{
/******************************************************

	recv() bytes and store them in a static buffer 
	temporarily before realloc() it
	
******************************************************/		

	//while(1)
	//{
		 
		ssize_t msg_length = 0;

		//int num_bytes = 0;
		//char static_buff[BYTES];
		 
		//char* outgoing_data = NULL;
		uint8_t completed = 0;
		int fd;
		
		printf("Thread ID: %ld\n\r", ((thread_info_t *)thread_data)->thread_id);
		
		//TODO: Rewrite this code to recv() and write() byte by byte
		
		char temp_buff;
		
		ret_val = pthread_mutex_lock (&mutex_aesdsocketdata_file);	
		
			
		fd = open(PATH_TO_FILE, O_WRONLY|O_APPEND , 0644);
		if(fd<0)
		{
			errnum = errno; //error logging			
			syslog(LOG_ERR, "open() returned error. The error was %s\n", strerror(errnum));
			
			printf("open() returned error %d\n\r", errnum);
			
			free(incoming_data);
			return_val = -1;
			return &return_val;
		}
		
		while(!completed)
		{
			msg_length = recv(((thread_info_t *)thread_data)->sockfd_connected, &temp_buff, 1, 0);
			if(msg_length <0)
			{
				errnum = errno;
				syslog(LOG_ERR, "recv() returned error. The error was %s\n", strerror(errnum));
				printf("No more data is available to read. recv() returned error %d\n\r", errnum);
				
				free(incoming_data);
				return_val = -1;
				return &return_val;
			}
			else if(msg_length == 0)
			{
				printf("No more messages\n\r");
				completed = 1;
				break;
			}
			
			if(temp_buff == '\n')
			{
				completed = 1;
				//break;
			}
			total_bytes++;
			
			ret_val = write(fd, &temp_buff, 1);
			if(ret_val == -1)
			{
				errnum = errno; //error logging			
				syslog(LOG_ERR, "write() returned error. The error was %s\n", strerror(errnum));
				
				printf("write() returned error %d\n\r", errnum);
				
				free(incoming_data);
				return_val = -1;
				return &return_val;
			}
		}
		
		ret_val = close(fd);
		if(ret_val == -1)
		{
			errnum = errno; //error logging			
			syslog(LOG_ERR, "close() returned error. The error was %s\n", strerror(errnum));
			
			printf("close() returned error %d\n\r", errnum);
			
			free(incoming_data);
			return_val = -1;
			return &return_val;
		}
		
		//TODO: Unlock mutex 
		pthread_mutex_unlock (&mutex_aesdsocketdata_file);

		/*while(!completed)
		{
			memset(&static_buff[0], 0, BYTES);
			
			msg_length = recv(((thread_info_t *)thread_data)->sockfd_connected, &static_buff[0], BYTES, 0); 
			if(msg_length <0)
			{
				errnum = errno;
				syslog(LOG_ERR, "recv() returned error. The error was %s\n", strerror(errnum));
				printf("No more data is available to read. recv() returned error %d\n\r", errnum);
				
				free(incoming_data);
				return_val = -1;
				return &return_val;
			}
			else if(msg_length == 0)
			{
				printf("No more messages\n\r");
				completed = 1;
				break;
			}
			
			int i =0;
			for(i = 0; i<BYTES; i++)
			{
				if(static_buff[i] == 10)
				{
					//num_bytes = i+1;
					//++i;
					completed = 1;
					break;
				}
				
			}
			
			//printf("sB %s ", &static_buff[0]);
			
			if(i == BYTES)
			{
				num_bytes = i;
			}
			else
			{
				num_bytes = ++i;
			}
			
			total_bytes += num_bytes; // (i+1)
			printf("total_bytes: %d\n\r", total_bytes);
			//printf("num_bytes: %d\n\r",num_bytes);
			
			incoming_data = (char *)realloc(incoming_data, (total_bytes));
			if(incoming_data == NULL)
			{
				printf("realloc failed\n\r");
				free(incoming_data);
				return_val = -1;
				return &return_val;
			}	
			
			//printf("data: %s", incoming_data);
			
			strncat(incoming_data, &static_buff[0],num_bytes);	
			printf("data: %s", incoming_data);	
		}
		//printf("length_recv: %ld\n\r", sizeof(incoming_data));
		printf("recv: %s", incoming_data);*/
	
/******************************************************

	writing the realloc() buffer into aesdsocketdata.txt
	using write()
	
******************************************************/	

			
		//TODO: Lock mutex or wait if in use by some other thread		
		/*ret_val = pthread_mutex_lock (&mutex_aesdsocketdata_file);	
		
			
		fd = open(PATH_TO_FILE, O_WRONLY|O_APPEND , 0644);
		if(fd<0)
		{
			errnum = errno; //error logging			
			syslog(LOG_ERR, "open() returned error. The error was %s\n", strerror(errnum));
			
			printf("open() returned error %d\n\r", errnum);
			
			free(incoming_data);
			return_val = -1;
			return &return_val;
		}
		
		//printf("total_bytes: %d\n\r", total_bytes);
		
		ret_val = write(fd, incoming_data, strlen(incoming_data));
		if(ret_val == -1)
		{
			errnum = errno; //error logging			
			syslog(LOG_ERR, "write() returned error. The error was %s\n", strerror(errnum));
			
			printf("write() returned error %d\n\r", errnum);
			
			free(incoming_data);
			return_val = -1;
			return &return_val;
		}
		else if(ret_val != strlen(incoming_data))
		{
			syslog(LOG_ERR, "Partial Write\n\r");
			
			printf("partial Write\n\r");
			
			//return -1;
		}
		printf("Written: %d\n\r", ret_val);
		ret_val = close(fd);
		if(ret_val == -1)
		{
			errnum = errno; //error logging			
			syslog(LOG_ERR, "close() returned error. The error was %s\n", strerror(errnum));
			
			printf("close() returned error %d\n\r", errnum);
			
			free(incoming_data);
			return_val = -1;
			return &return_val;
		}
		
		//TODO: Unlock mutex 
		pthread_mutex_unlock (&mutex_aesdsocketdata_file);	*/
		
		
/******************************************************

	open() file, read() all the characters from it, 
	then send() it
	
******************************************************/	

		
		
		int bytes = total_bytes; //strlen(incoming_data);
		char buff;
		
		//printf("bytes: %d\n\r", bytes);

		//TODO: Lock mutex
		pthread_mutex_lock (&mutex_aesdsocketdata_file);	
		
		fd = open(PATH_TO_FILE, O_RDONLY, 0644);
		if(fd<0)
		{
			errnum = errno; //error logging			
			syslog(LOG_ERR, "open() returned error. The error was %s\n", strerror(errnum));
			
			printf("open() returned error %d\n\r", errnum);
			
			free(incoming_data);
			return_val = -1;
			return &return_val;
		}
		
		//TODO: replace lseek() because in multithreading applications, each thread's lseek call will vary the absolute file offset.
		// Not an issue because open() and close() calls are also locked by mutex
		//ret_val = lseek(fd, 0, SEEK_SET);
		if(ret_val == 1)
		{
			errnum = errno; //error logging			
			syslog(LOG_ERR, "lseek() returned error. The error was %s\n", strerror(errnum));
			
			printf("lseek() returned error %d\n\r", errnum);
		}
		//printf("lseek(): %d\n\r", ret_val);
		//printf("send: ");
		while(bytes--)
		{
			//outgoing_data = (char *)malloc(total_bytes*(sizeof(char)));
			
			
			ret_val = read(fd, &buff, 1);
			if(ret_val == -1)
			{
				errnum = errno; //error logging			
				syslog(LOG_ERR, "read() returned error. The error was %s\n", strerror(errnum));
				
				printf("read() returned error %d\n\r", errnum);
				
				free(incoming_data);
				return_val = -1;
				return &return_val;
			}
			
			//printf("ret_val_read: %d\n\r", ret_val);
			//printf("%s", &buff);
			
			//TODO: Lock mutex or wait if in use by some other thread
			
			ret_val = send(((thread_info_t *)thread_data)->sockfd_connected, &buff, 1, 0);
			if(ret_val == -1)
			{
				errnum = errno; //error logging			
				syslog(LOG_ERR, "send() returned error. The error was %s\n", strerror(errnum));
				
				printf("send() returned error %d\n\r", errnum);
				
				free(incoming_data);
				return_val = -1;
				return &return_val;
			}	
			//printf("ret_val_rsend: %d\n\r", ret_val);
			
			//TODO: Unlock mutex 
			
		}
		
		/*char *outgoing = (char *)malloc(bytes*sizeof(char));
		
		ret_val = read(fd, outgoing, bytes);
		if(ret_val == -1)
		{
			errnum = errno; //error logging			
			syslog(LOG_ERR, "read() returned error. The error was %s\n", strerror(errnum));
			
			printf("read() returned error %d\n\r", errnum);
			
			free(incoming_data);
			return_val = -1;
			return &return_val;
		}
		
		printf("outgoing : %s", outgoing);
		
		ret_val = send(((thread_info_t *)thread_data)->sockfd_connected, outgoing, bytes, 0);
		if(ret_val == -1)
		{
			errnum = errno; //error logging			
			syslog(LOG_ERR, "send() returned error. The error was %s\n", strerror(errnum));
			
			printf("send() returned error %d\n\r", errnum);
			
			free(incoming_data);
			return_val = -1;
			return &return_val;
		}	
		else if(ret_val != bytes)
		{
			printf("Complete string not sent\n\r");
		}*/
		
		
		ret_val = close(fd);
		if(ret_val == -1)
		{
			errnum = errno; //error logging			
			syslog(LOG_ERR, "close() returned error. The error was %s\n", strerror(errnum));
			
			printf("close() returned error %d\n\r", errnum);
			freeaddrinfo(servinfo);
			remove(PATH_TO_FILE);
			free(incoming_data);
			return_val = -1;
			return &return_val;
		}
			
		//free(incoming_data);
		
		syslog(LOG_DEBUG, "Closed Connection from %s\n\r", ip_addr);
		printf("Closed Connection from %s\n\r", ip_addr);
		
		//TODO: Unlock mutex
		pthread_mutex_unlock (&mutex_aesdsocketdata_file);	
	//}
	
	((thread_info_t *)thread_data)->terminate_thread_flag =1;
	return_val = 0;
	return &return_val;
		
}


int main(int argc , char** argv)
{
	
	/*Start syslog daemon*/
    openlog("aesdsocket", LOG_USER, LOG_DEBUG|LOG_ERR); 

	int num_threads =0;

		

	
	
	signal(SIGINT, sig_handler);
	signal(SIGTERM, sig_handler);
	signal(SIGKILL, sig_handler);
	
/******************************************************

	creating Linked List
	
******************************************************/

	//thread_info_t *datap = NULL;
	
	SLIST_HEAD(slisthead, thread_info) head;
	//struct slisthead *headp;
	SLIST_INIT(&head);
	//headp = head;


/******************************************************

	creating mutex
	
******************************************************/

	
	
	/*unlock it ?*/

	
/******************************************************

	 Setup_comm()
	
******************************************************/

	if(setup_comm() == -1)
	{
		return -1;
	}
	
	/* what happens if no connection on listen() */

	
/******************************************************

	creating a daemon only if -d is given as argument
	daemon();
	listen()...
	
******************************************************/

	if((argc >1) && (strcmp((char*)argv[1], "-d") == 0))
	{
		ret_val = daemon(0,0);
		if(ret_val == -1)
		{
			errnum =errno;
			syslog(LOG_ERR, "daemon() returned error. The error was %s\n", strerror(errnum));
			printf("Error: daemon() returned error %d\n\r", errnum);
			
		}
	}

	//int z = 1;
	while(1)	
	{	
		incoming_data = (char *)malloc(BYTES*sizeof(char));
		if(incoming_data == NULL)
		{
			printf("malloc failed\n\r");
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
			
			free(incoming_data);
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
			
			//free(incoming_data);
			return -1;
		}
		

		
		// print the IP address of client
		struct sockaddr_in *addr = (struct sockaddr_in *)&servinfo_connectingaddr;
		
		inet_ntop(AF_INET, &(addr->sin_addr), ip_addr, INET_ADDRSTRLEN);
		syslog(LOG_DEBUG, "Accepted Connection from %s\n\r", ip_addr);
		printf("Accepting connection from %s\n\r", ip_addr);
		
/******************************************************

	Create thread, store thread id in linked list
	
******************************************************/

		thread_info_t *ptr; 
		ptr = (thread_info_t *)malloc(sizeof(thread_info_t));
		
		pthread_create(&(ptr->thread_id), NULL, thread_func, ptr);
		
		SLIST_INSERT_HEAD(&head, ptr, entries);
		ptr->terminate_thread_flag = 0;
		//TODO: Enter connection parameter from socket
		ptr->sockfd_connected = socket_fd_connected;
		num_threads++;

/******************************************************

	Join thread, check if thread has terminated
	and then remove thread ID from linked list
	
******************************************************/

		//pthread_join(*(ptr->thread_id), NULL);	
		/* join only if the terminate flag is set */

		/* loop through the list to check if terminate flag is set 
			remove node from list and free()*/
			
		thread_info_t *ptr_temp = NULL;
		thread_info_t *p = NULL;
		ptr_temp = SLIST_FIRST(&head);
		
		int temp = num_threads;

		while(temp--)
		{
			if(ptr_temp->terminate_thread_flag == 1)
			{
				close(ptr_temp->sockfd_connected);
				pthread_join(ptr_temp->thread_id, NULL);
				p = ptr_temp;
				ptr_temp = SLIST_NEXT(ptr_temp, entries);
				SLIST_REMOVE(&head, p, thread_info, entries);
				free(p);
				num_threads--;
			}
		}
		//printf("END\n\r");
	
	}
	
	
	close(socket_fd);
	unlink(PATH_TO_FILE);

}
