#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

// function to: wait, obtain mutex, wait, release mutex as described by thread_data structure
void* threadfunc(void* thread_param)
{
    // Obtain the thread arguments
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    if (thread_func_args == NULL) {
        ERROR_LOG("thread_func_args is NULL");
        return thread_param; // nothing we can do
    }

    // Default to failure; set success only when all steps complete
    thread_func_args->thread_complete_success = false;

    // 1) Wait before attempting to obtain the mutex
    if (thread_func_args->wait_to_obtain_ms > 0) {
        // usleep takes microseconds
        usleep((unsigned int)thread_func_args->wait_to_obtain_ms * 1000U);
    }

    // 2) Obtain (lock) the mutex
    if (pthread_mutex_lock(thread_func_args->mutex) != 0) {
        ERROR_LOG("pthread_mutex_lock failed");
        return thread_param;
    }

    // 3) Wait while holding the mutex
    if (thread_func_args->wait_to_release_ms > 0) {
        usleep((unsigned int)thread_func_args->wait_to_release_ms * 1000U);
    }

    // 4) Release (unlock) the mutex
    if (pthread_mutex_unlock(thread_func_args->mutex) != 0) {
        ERROR_LOG("pthread_mutex_unlock failed");
        return thread_param;
    }

    // If we got here, all steps succeeded
    thread_func_args->thread_complete_success = true;

    return thread_param;
}


/**
 * function to: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
 * using threadfunc() as entry point.
 * return true if successful.
 */
bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{

    if (thread == NULL || mutex == NULL) {
        ERROR_LOG("Invalid arguments: thread or mutex is NULL");
        return false;
    }

    struct thread_data *data = (struct thread_data *)calloc(1, sizeof(struct thread_data));
    if (data == NULL) {
        ERROR_LOG("Failed to allocate thread_data");
        return false;
    }

    // Initialize thread argument fields
    data->mutex = mutex;
    data->wait_to_obtain_ms = wait_to_obtain_ms;
    data->wait_to_release_ms = wait_to_release_ms;
    data->thread_complete_success = false;

    int rc = pthread_create(thread, NULL, threadfunc, (void *)data);
    if (rc != 0) {
        ERROR_LOG("pthread_create failed (rc=%d)", rc);
        free(data);
        return false;
    }

    // Note: We intentionally DO NOT free(data) here because the thread uses it.
    return true;
}