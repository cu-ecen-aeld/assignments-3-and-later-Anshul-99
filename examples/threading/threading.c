#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)  //variable argument

void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    //struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    
    int ret;
    int errnum;
    
    
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    //printf("thread_param: %p\n\r", thread_func_args);
    //printf("mutex_info: %p\n\r", thread_func_args->mutex_info);
    
    printf("Thread start. Going to sleep for %d\n\r", thread_func_args->wait_obtain_mutex);
    
    ret = usleep((thread_func_args->wait_obtain_mutex)*1000);
    if(ret != 0)
    {
    	//error
    	errnum = errno;
    	/* mutex_lock() call would technically be blocked until the mutex is available. 
    	So this would always be zero but still no harm in checking it. 
    	In case there are other bugs.*/
    	printf("Error: %d \n\r", errnum);
    }
    
    printf("Obtaining mutex. Going to sleep for %d \n\r", thread_func_args->wait_release_mutex);
    
    ret = pthread_mutex_lock(thread_func_args->mutex_info);
    printf("Obtained mutex\n\r");
    if(ret != 0)
    {
    	//error
    	errnum = errno;
    	/* mutex_lock() call would technically be blocked until the mutex is available. 
    	So this would always be zero but still no harm in checking it. 
    	In case there are other bugs.*/
    	printf("Error: %d \n\r", errnum);
    }
    
    printf("Going to sleep\n\r");
    //wait()
    ret = usleep((thread_func_args->wait_release_mutex)*1000);
    if(ret != 0)
    {
    	//error
    	errnum = errno;
    	/* mutex_lock() call would technically be blocked until the mutex is available. 
    	So this would always be zero but still no harm in checking it. 
    	In case there are other bugs.*/
    	printf("Error: %d \n\r", errnum);
    }
    printf("Unlocking mutex\n\r");
    
    ret = pthread_mutex_unlock(thread_func_args->mutex_info);
    if(ret != 0)
    {
    	//error
    	/* mutex_unlock() call would immediately release the mutex and return zero.
    	Wouldn't harm to still check*/
    	printf("Error: %d \n\r", errnum);
    }
    printf("End thread\n\r");
    
    thread_func_args->thread_complete_success = true;
    
    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     * 
     * See implementation details in threading.h file comment block
     */
     
     int ret_val;
     //pthread_t thread1;
     int errnum;
     
     //dynamically allocate memory for thread_data
     struct thread_data *ptr = NULL;
     ptr = (struct thread_data *)malloc(sizeof(struct thread_data));
     //printf("Ptr %p\n\r", ptr);
     

     //assign mutex
     ptr->mutex_info = mutex;
     ptr->wait_obtain_mutex = wait_to_obtain_ms;
     ptr->wait_release_mutex = wait_to_release_ms;
     ptr->thread_ID = thread;
     printf("malloced successfully \n\r");
     //printf("mutex %p\n\r", mutex);
     
     //create thread
     ret_val = pthread_create(ptr->thread_ID, NULL, threadfunc, ptr);

     
     //check ret_val for errors
     if(ret_val != 0)
     {
     	//error
     	errnum = errno;
     	//use error log macro
     	printf("Error: %d \n\r", errnum);
     	return false;
     }
      printf("Created thread\n\r");
     
	 //join threads
	 /*ret_val = pthread_join(thread1, NULL);
     printf("Joined thread\n\r");
     if(ret_val != 0)
     {
     	//error
     	errnum = errno;
     	//use error log macro
     	printf("Error: %d \n\r", errnum);
     	return false;
     }

     
     //terminate thread?
     
     // free() ptr after thread completes execution
     free(ptr);*/
      //printf("Memory freed\n\r");
    
     
    return true;
}

