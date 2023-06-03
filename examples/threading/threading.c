#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>


// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    	//wait time. Usleep requires microseconds inputs
    	usleep((thread_func_args->wait_to_obtain_ms)*1000);
    	//obtain mutex
    if (pthread_mutex_lock(thread_func_args->mutex)!=0){
    	printf("failure to lock mutex");
    	}
    	//wait	
    	usleep((thread_func_args->wait_to_release_ms)*1000);
    	//release mutex
    if (pthread_mutex_unlock(thread_func_args->mutex)!=0){
    	printf("failure to unlock mutex");
	}
	
	thread_func_args->thread_complete_success = true;
	return thread_param;
   }


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**allocate memory for thread_data,**/
     struct thread_data *data;
     data=malloc(sizeof(struct thread_data));
     
    /**setup mutex and wait arguments, **/
     data->mutex = mutex;
     data->wait_to_obtain_ms = wait_to_obtain_ms;
     data->wait_to_release_ms = wait_to_release_ms;
     
     /**pass thread_data to created thread using threadfunc() as entry point**/
     int PTC=0; /**variable to store retun of pthread func**/
     PTC=pthread_create(thread, NULL, threadfunc, (void *)data);
    
     if(PTC < 0)
        printf("pthread_create error");
    else
    {
        printf("pthread_create successful");
        return true;
    }
    return false;}
    
   
   


