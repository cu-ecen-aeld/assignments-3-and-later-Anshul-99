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
#include <time.h>
#include <sys/time.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>


#define SERVICE_PORT "9000"
#define PATH_TO_FILE "/var/tmp/aesdsocketdata.txt"
#define BYTES 128

SLIST_HEAD(slisthead, thread_info) head;

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
int num_threads =0;
int z =1;


char time_ptr[40];




typedef struct thread_info thread_info_t;
struct thread_info
{
	pthread_t thread_id;
	int terminate_thread_flag;
	SLIST_ENTRY(thread_info) entries;
	int sockfd_connected;
};

pthread_mutex_t mutex_aesdsocketdata_file = PTHREAD_MUTEX_INITIALIZER;

//TODO: define a function that handles the cleaning up and is called when returning due to error

static void sig_handler (int signo)
{
	if((signo == SIGINT) || (signo == SIGTERM) /*|| (signo == SIGKILL)*/)
	{
		printf("Num threads: %d\n\r", num_threads);
		
		thread_info_t *p = NULL;
		while(!SLIST_EMPTY(&head))
		{
			p = SLIST_FIRST(&head);
			SLIST_REMOVE_HEAD(&head, entries);
			pthread_join(p->thread_id, NULL);
			free(p);
		}
		
		unlink(PATH_TO_FILE);
		close(socket_fd_connected);
		close(socket_fd);
		z =0;
	}
}

void alarm_handler(int signum)
{
	time_t rawtime = time(NULL);
	struct tm *ptm = localtime(&rawtime);

	if(ptm == NULL)
	{
		errnum =errno;
		syslog(LOG_ERR, "localtime() returned error. The error was %s\n", strerror(errnum));
		printf("Error: localtime() returned error %d\n\r", errnum);
	}
	strftime(&time_ptr[0], 40, "%a %b %d %Y %H:%M:%S", ptm);

	printf("Time: %s\n\r", &time_ptr[0]);
	
	pthread_mutex_lock (&mutex_aesdsocketdata_file);

	int fd = open(PATH_TO_FILE, O_WRONLY|O_APPEND , 0644);
	if(fd<0)
	{
		errnum = errno; //error logging			
		syslog(LOG_ERR, "open() returned error. The error was %s\n", strerror(errnum));
		
		printf("open() returned error %d\n\r", errnum);
	}

	
	char *timestamp = "timestamp:";
	
	ret_val = write(fd, timestamp, (sizeof(timestamp)+2));
	if(ret_val == -1)
	{
		errnum = errno; //error logging			
		syslog(LOG_ERR, "write() returned error. The error was %s\n", strerror(errnum));
		
		printf("write() returned error %d\n\r", errnum);
	}
	total_bytes+= ret_val;
	
	int i = 0;
	while(time_ptr[i] != '\0')
	{
		printf("%c", time_ptr[i]);
		ret_val = write(fd, &time_ptr[i], 1);
		if(ret_val == -1)
		{
			errnum = errno; //error logging			
			syslog(LOG_ERR, "write() returned error. The error was %s\n", strerror(errnum));
			
			printf("write() returned error %d\n\r", errnum);
		}
		i++;
		total_bytes++;
	}
	
	ret_val = write(fd, "\n", 1);
	if(ret_val == -1)
	{
		errnum = errno; //error logging			
		syslog(LOG_ERR, "write() returned error. The error was %s\n", strerror(errnum));
		
		printf("write() returned error %d\n\r", errnum);
	}
	total_bytes++;

	ret_val = close(fd);
	if(ret_val == -1)
	{
		errnum = errno; //error logging			
		syslog(LOG_ERR, "close() returned error. The error was %s\n", strerror(errnum));
		
		printf("close() returned error %d\n\r", errnum);
	}
	
	syslog(LOG_DEBUG, "TIMESTAMP: %s\n", &time_ptr[0]);
	// Unlock mutex 
	pthread_mutex_unlock (&mutex_aesdsocketdata_file);
	return;
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


void * thread_func(void *thread_data)
{
/******************************************************

	recv() bytes and store them in a static buffer 
	temporarily before realloc() it
	
******************************************************/		

	ssize_t msg_length = 0;
	uint8_t completed = 0;
	int fd;
	char temp_buff;
	
	printf("Thread ID: %ld\n\r", ((thread_info_t *)thread_data)->thread_id);
	
	printf("Thread: %ld  Locked mutex\n\r", ((thread_info_t *)thread_data)->thread_id);
	pthread_mutex_lock (&mutex_aesdsocketdata_file);	
		
	printf("Thread: %ld  Opened file\n\r", ((thread_info_t *)thread_data)->thread_id);
	fd = open(PATH_TO_FILE, O_WRONLY|O_APPEND , 0644);
	if(fd<0)
	{
		errnum = errno; //error logging			
		syslog(LOG_ERR, "open() returned error. The error was %s\n", strerror(errnum));
		
		printf("open() returned error %d\n\r", errnum);
		
		return_val = -1;
		return &return_val;
	}
		
	printf("Thread: %ld  Entering recv()/write() loop\n\r", ((thread_info_t *)thread_data)->thread_id);
	while(!completed)
	{
		msg_length = recv(((thread_info_t *)thread_data)->sockfd_connected, &temp_buff, 1, 0);
		if(msg_length <0)
		{
			errnum = errno;
			syslog(LOG_ERR, "recv() returned error. The error was %s\n", strerror(errnum));
			printf("No more data is available to read. recv() returned error %d\n\r", errnum);
			
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
		}
		total_bytes++;
		
		ret_val = write(fd, &temp_buff, 1);
		if(ret_val == -1)
		{
			errnum = errno; //error logging			
			syslog(LOG_ERR, "write() returned error. The error was %s\n", strerror(errnum));
			
			printf("write() returned error %d\n\r", errnum);
			
			return_val = -1;
			return &return_val;
		}
	}
	printf("Thread: %ld  Exited recv()/write() loop\n\r", ((thread_info_t *)thread_data)->thread_id);
	
	ret_val = close(fd);
	if(ret_val == -1)
	{
		errnum = errno; //error logging			
		syslog(LOG_ERR, "close() returned error. The error was %s\n", strerror(errnum));
		
		printf("close() returned error %d\n\r", errnum);
		
		return_val = -1;
		return &return_val;
	}
	printf("Thread: %ld  Closed file\n\r", ((thread_info_t *)thread_data)->thread_id);
	
	// Unlock mutex 
	pthread_mutex_unlock (&mutex_aesdsocketdata_file);
	printf("Thread: %ld  Unlocked mutex\n\r", ((thread_info_t *)thread_data)->thread_id);

		
	

		
		
/******************************************************

	open() file, read() all the characters from it, 
	then send() it
	
******************************************************/	

		
		
	int bytes = total_bytes; 
	char buff;


	// Lock mutex
	pthread_mutex_lock (&mutex_aesdsocketdata_file);	
	printf("Thread: %ld  Locked mutex\n\r", ((thread_info_t *)thread_data)->thread_id);
	
	printf("Thread: %ld  Opened file\n\r", ((thread_info_t *)thread_data)->thread_id);
	fd = open(PATH_TO_FILE, O_RDONLY, 0644);
	if(fd<0)
	{
		errnum = errno; //error logging			
		syslog(LOG_ERR, "open() returned error. The error was %s\n", strerror(errnum));
		
		printf("open() returned error %d\n\r", errnum);
		
		return_val = -1;
		return &return_val;
	}
		
	printf("Thread: %ld  lseek()\n\r", ((thread_info_t *)thread_data)->thread_id);
	ret_val = lseek(fd, 0, SEEK_SET);
	if(ret_val == 1)
	{
		errnum = errno; //error logging			
		syslog(LOG_ERR, "lseek() returned error. The error was %s\n", strerror(errnum));
		
		printf("lseek() returned error %d\n\r", errnum);
	}

	printf("Thread: %ld  enter read()/send() loop \n\r", ((thread_info_t *)thread_data)->thread_id);
	while(bytes--)
	{
		ret_val = read(fd, &buff, 1);
		if(ret_val == -1)
		{
			errnum = errno; //error logging			
			syslog(LOG_ERR, "read() returned error. The error was %s\n", strerror(errnum));
			
			printf("read() returned error %d\n\r", errnum);
			
			return_val = -1;
			return &return_val;
		}
		
		ret_val = send(((thread_info_t *)thread_data)->sockfd_connected, &buff, 1, 0);
		if(ret_val == -1)
		{
			errnum = errno; //error logging			
			syslog(LOG_ERR, "send() returned error. The error was %s\n", strerror(errnum));
			
			printf("send() returned error %d\n\r", errnum);
			
			return_val = -1;
			return &return_val;
		}				
	}
	printf("Thread: %ld  exited read()/send() loop\n\r", ((thread_info_t *)thread_data)->thread_id);
		
	printf("Thread: %ld  file closed\n\r", ((thread_info_t *)thread_data)->thread_id);
	ret_val = close(fd);
	if(ret_val == -1)
	{
		errnum = errno; //error logging			
		syslog(LOG_ERR, "close() returned error. The error was %s\n", strerror(errnum));
		
		printf("close() returned error %d\n\r", errnum);
		freeaddrinfo(servinfo);
		remove(PATH_TO_FILE);
		return_val = -1;
		return &return_val;
	}
	
	
	
	
	// Unlock mutex
	pthread_mutex_unlock (&mutex_aesdsocketdata_file);
	printf("Thread: %ld  Unlocked mutex\n\r", ((thread_info_t *)thread_data)->thread_id);	

	((thread_info_t *)thread_data)->terminate_thread_flag =1;
	syslog(LOG_DEBUG, "Closed Connection from %s\n\r", ip_addr);
	printf("Closed Connection from %s\n\r", ip_addr);
	return_val = 0;
	return &return_val;
		
}


int main(int argc , char** argv)
{
	
	/*Start syslog daemon*/
    openlog("aesdsocket", LOG_USER, LOG_DEBUG|LOG_ERR); 

	

/******************************************************

	signal()
	
******************************************************/
	signal(SIGINT, sig_handler);
	signal(SIGTERM, sig_handler);
	//signal(SIGKILL, sig_handler);
	signal(SIGALRM, alarm_handler);
	
/******************************************************

	creating Linked List
	
******************************************************/


	SLIST_INIT(&head);


/******************************************************

	creating mutex
	
******************************************************/

	
	//created as a global variable

	
/******************************************************

	 Setup_comm()
	
******************************************************/

	if(setup_comm() == -1)
	{
		return -1;
	}
	

	
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


	while(z)	
	{	
/******************************************************

	listen()
	
******************************************************/

		ret_val = listen(socket_fd, 10); /* backlog value might cause problems */
		if(ret_val == -1)
		{
			errnum = errno;
			syslog(LOG_ERR, "listen() returned error. The error was %s\n", strerror(errnum));
			
			printf("Error: listen() returned error %d\n\r", errnum);
			
			return -1;
		}
		
		
		
/******************************************************

	accept()
	
******************************************************/
		addr_size = sizeof servinfo_connectingaddr;
		socket_fd_connected = accept(socket_fd, (struct sockaddr *)&servinfo_connectingaddr, &addr_size);
		if(socket_fd_connected == -1)
		{
			if(z ==0)
			{
				close(socket_fd);
				return 0;
			}
			errnum = errno;
			syslog(LOG_ERR, "accept() returned error. The error was %s\n", strerror(errnum));
			
			printf("Error: accept() returned error %d\n\r", errnum);
			close(socket_fd);
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

		thread_info_t *ptr = (thread_info_t *)malloc(sizeof(thread_info_t));
		SLIST_INSERT_HEAD(&head, ptr, entries);
		ptr->terminate_thread_flag = 0;
		//TODO: Enter connection parameter from socket
		ptr->sockfd_connected = socket_fd_connected;
		pthread_create(&(ptr->thread_id), NULL, thread_func, ptr);
		num_threads++;
		syslog(LOG_DEBUG, "thread allocated %ld\n", ptr->thread_id);
		
		struct itimerval delay;
	
		delay.it_value.tv_sec = 10;
		delay.it_value.tv_usec = 0;
		delay.it_interval.tv_sec = 10;
		delay.it_interval.tv_usec =0;
		ret_val = setitimer(ITIMER_REAL, &delay, NULL);
		if(ret_val)
		{
			errnum =errno;
			syslog(LOG_ERR, "setitimer() returned error. The error was %s\n", strerror(errnum));
			printf("Error: setitimer() returned error %d\n\r", errnum);
		}

/******************************************************

	Join thread, check if thread has terminated
	and then remove thread ID from linked list
	
******************************************************/
	
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
				syslog(LOG_DEBUG, "thread deallocated %ld\n", ptr_temp->thread_id);
				close(ptr_temp->sockfd_connected);
				pthread_join(ptr_temp->thread_id, NULL);
				p = ptr_temp;
				ptr_temp = SLIST_NEXT(ptr_temp, entries);
				SLIST_REMOVE(&head, p, thread_info, entries);
				free(p);
				num_threads--;
				
				
			}
			else
			{
				ptr_temp = SLIST_NEXT(ptr_temp, entries);
			}
		}
	
	}
	
	
	close(socket_fd);
	unlink(PATH_TO_FILE);

}
