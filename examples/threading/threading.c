#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    //struct thread_data* thread_func_args = (struct thread_data *) thread_param;
void* threadfunc(void* thread_param)
{
    int res;
    bool success = false;
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    
    usleep( thread_func_args->wait_to_obtain_ms * 1000);
    res = pthread_mutex_lock( thread_func_args->mutex );
    if (res != 0){
        printf("Failed to lock mutex with result %d \n", res);
    }else{    
        success = true;
        printf("Wait to release ms %d \n", thread_func_args->wait_to_release_ms);
        usleep( thread_func_args->wait_to_release_ms *1000);
    }
    res = pthread_mutex_unlock( thread_func_args->mutex );
    if (res != 0){
        printf("Failed to release mutex with result %d \n", res);
        success = false;
    }
    thread_func_args->thread_complete_success = success;
    return thread_param;
}

    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */

bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    int res;
    struct thread_data * t_data;
    t_data = (struct thread_data*)malloc(sizeof(struct thread_data));
    if (t_data == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        return false;
    }
    t_data->wait_to_obtain_ms = wait_to_obtain_ms;
    t_data->wait_to_release_ms = wait_to_release_ms;
    t_data->mutex = mutex;  
    res = pthread_create(thread, NULL, threadfunc,(void*) t_data);
    if (res != 0){
        printf("Error creating thread.\n");
        return false;
    }
    return true;
}

