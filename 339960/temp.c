#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>



struct blocked_thread
{
    int* thread_id;
    struct blocked_thread* next;
};

struct batcher
{
    int count;
    int remaining;
    struct blocked_thread* blocked_threads_head;
};


typedef struct Node {
    sem_t sem;              // Semaphore for signaling
    struct Node* next;     // Pointer to the next node
    int id;                 // Identifier for the thread
} Node;


void* worker_function(void* arg) {
    Node* current_node = (Node*)arg;

    // Wait on the current node's semaphore
    if (sem_wait(&current_node->sem) != 0) {
        perror("sem_wait failed");
        pthread_exit(NULL);
    }

    // Perform the task upon waking up
    printf("Thread %d woke up.\n", current_node->id);
    fflush(stdout); // Ensure immediate output

    // Signal the next thread's semaphore if it exists
    if (current_node->next != NULL) {
        if (sem_post(&current_node->next->sem) != 0) {
            perror("sem_post failed");
            pthread_exit(NULL);
        }
    }

    pthread_exit(NULL);
}
