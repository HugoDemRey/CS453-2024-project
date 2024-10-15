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
    pthread_mutex_t* enter_lock;
} batcher;


typedef struct blocked_thread {
    sem_t sem;                       // Semaphore for signaling
    struct blocked_thread* next;     // Pointer to the next node
    int id;                          // Identifier for the thread
} blocked_thread;


void wake_up_threads(batcher* batcher) {
    sem_post(&batcher->blocked_threads_head->sem);
}


void enter_batcher(batcher* batcher, blocked_thread* blocked_thread) {
    pthread_mutex_lock(batcher->enter_lock);


    if (batcher->remaining == 0) {
        // Maybe use atomic operations of C
        batcher->remaining++;
        pthread_mutex_unlock(batcher->enter_lock);
        return;
    }

    if (batcher->blocked_threads_tail == NULL) {
        batcher->blocked_threads_head = blocked_thread;
        batcher->blocked_threads_tail = blocked_thread;
        blocked_thread->next = NULL;
    } else {
        batcher->blocked_threads_tail->next = blocked_thread;
        batcher->blocked_threads_tail = blocked_thread;
        blocked_thread->next = NULL;
    }

    pthread_mutex_unlock(batcher->enter_lock);

    
    /* Make the current thread sleep using sem (sem_t) which is contained in blocked_thread, wake him up when sem tells the thread to wake up */
    printf("Thread %d is going to sleep.\n", blocked_thread->id);
    sem_wait(&blocked_thread->sem);
    printf("Thread %d woke up.\n", blocked_thread->id);

    // Maybe use atomic operations of C
    batcher->remaining++;

    /* 
    FIXME : If a thread assigns itself right after the IF condition as the next element of the tail, it will never be woken up.
    Solution 1: lock the mutex again and check if the next element is the current thread, if so, wake it up.
    */
    // Solution 1
    pthread_mutex_lock(batcher->enter_lock);

    /* Wake up the next thread in the list */
    if (blocked_thread->next == NULL) {
        // Solution 1
        batcher->blocked_threads_tail = NULL;
        blocked_thread = NULL;
        return;
    }
    printf("Thread %d is waking up the next thread\n", blocked_thread->id);
    sem_post(&blocked_thread->next->sem);
    blocked_thread = NULL;

    // Solution 1
    pthread_mutex_unlock(batcher->enter_lock);

}

void leave_batcher(batcher* batcher) {
    int remaining = batcher->remaining;
    if (remaining == 1) {

        /* Update the memory since no thread is able to access it. I.e. all other threads are sleeping */
        
        /* 
        We would maybe want to use a lock to prevent a new process to enter the batcher and bypass the semaphore by seeing remaining == 0
        but it is not necessary since if a new process arrives at this moment, it will not be blocked and work with the other processes and also increment "remaining" by one as expected.
        */
        wake_up_threads(batcher);
        batcher->remaining--;
        return;
    }
    // Maybe use atomic operations of C
    batcher->remaining--;
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
