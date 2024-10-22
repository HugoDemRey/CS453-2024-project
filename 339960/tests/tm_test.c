#include "tm.h"
#include <stdio.h>
#include <stdlib.h>


blocked_thread* create_blocked_thread(int id){
    blocked_thread* b = malloc(sizeof(blocked_thread));
    if (b == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    if (sem_init(&b->sem, 0, 0) != 0) {
        perror("sem_init");
        exit(EXIT_FAILURE);
    }
    b->next = NULL;
    b->id = id;
    return b;
}

typedef struct {
    batcher* b;
    blocked_thread* t;
} thread_arg;

void* enter_batcher_thread(void* arg) {
    thread_arg* ta = (thread_arg*)arg;

    // Add a delay
    int delay = ta->t->id + 3; // delay corresponds to the id of the thread + 3
    //printf("Thread %d starts executing with %d s of delay \n\n", ta->t->id, delay);
    //sleep(delay);
    
    enter_batcher(ta->b, ta->t);
    
    
    int delay2 = 3;
    printf("Thread %d is done, it will leave the batcher in %d seconds \n\n", ta->t->id, delay2);
    sleep(delay2);

    leave_batcher(ta->b);

    return NULL;
}


int main(void) {

    int nb_threads = 3;

    // FIXME : the test is not correct.
    batcher* b = init_batcher();
    print_batcher(b);
    pthread_t threads[nb_threads];
    blocked_thread* t[nb_threads];
    thread_arg args[nb_threads];

    for (int i = 0; i < nb_threads; i++) {
        t[i] = create_blocked_thread(i+1);
    }

    for (int i = 0; i < nb_threads; i++) {
        args[i].b = b;
        args[i].t = t[i];
        pthread_create(&threads[i], NULL, enter_batcher_thread, (void*)&args[i]);
    }

    for (int i = 0; i < nb_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    destroy_batcher(b);
    b = NULL;

    return 1;
}

