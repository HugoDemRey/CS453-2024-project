#include "tm.h"
#include <stdio.h>
#include <stdlib.h>


blocked_thread* create_blocked_thread(int id){
    blocked_thread* b = malloc(sizeof(blocked_thread));
    sem_init(&b->sem, 0, 0);
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
    enter_batcher(ta->b, ta->t);
    return NULL;
}


int main(void) {

    // FIXME : the test is not correct.
    batcher* b = init_batcher();
    pthread_t threads[2];
    blocked_thread* t[2];
    thread_arg args[2];

    t[0] = create_blocked_thread(1);
    t[1] = create_blocked_thread(2);

    for (int i = 0; i < 2; i++) {
        args[i].b = b;
        args[i].t = t[i];
        pthread_create(&threads[i], NULL, enter_batcher_thread, (void*)&args[i]);
    }

    
    for (int i = 0; i < 5; i++) {
        pthread_create(&threads[i], NULL, enter_batcher_thread, (void*)t[i]);
    }

    for (int i = 0; i < 5; i++) {
        pthread_join(threads[i], NULL);
    }

    destroy_batcher(b);
    b = NULL;

    return 1;
}

