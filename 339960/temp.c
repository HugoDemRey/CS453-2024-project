#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>


typedef struct batcher
{
    int count;
    int remaining;
    struct blocked_thread* blocked_threads_head;
    struct blocked_thread* blocked_threads_tail;
} batcher;


typedef struct blocked_thread {
    sem_t sem;              // Semaphore for signaling
    struct blocked_thread* next;     // Pointer to the next node
    int id;                 // Identifier for the thread
} blocked_thread;


void wake_up_threads(batcher* batcher) {
    
}


void enter_batcher(batcher* batcher, blocked_thread* blocked_thread) {
    pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_lock(&lock);

    // If the blocked_threads is empty.
    if (batcher->blocked_threads_tail == NULL) {
        batcher->blocked_threads_head = blocked_thread;
        batcher->blocked_threads_tail = blocked_thread;
        blocked_thread->next = NULL;
        pthread_mutex_unlock(&lock);
        return;
    }

    batcher->blocked_threads_tail->next = blocked_thread;
    batcher->blocked_threads_tail = blocked_thread;
    blocked_thread->next = NULL;

    pthread_mutex_unlock(&lock);

    /* Make the current thread sleep using sem (sem_t) which is contained in blocked_thread, wake him up when sem tells the thread to wake up */

}

void leave_batcher(batcher* batcher) {
    /* ... */
}



void* worker_function(void* arg) {
    blocked_thread* current_thread = (blocked_thread*)arg;

    // Wait on the current node's semaphore
    if (sem_wait(&current_thread->sem) != 0) {
        perror("sem_wait failed");
        pthread_exit(NULL);
    }

    // Perform the task upon waking up
    printf("Thread %d woke up.\n", current_thread->id);
    fflush(stdout); // Ensure immediate output

    // Signal the next thread's semaphore if it exists
    if (current_thread->next != NULL) {
        if (sem_post(&current_thread->next->sem) != 0) {
            perror("sem_post failed");
            pthread_exit(NULL);
        }
    }

    pthread_exit(NULL);
}
