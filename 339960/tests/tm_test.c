#include "tm.h"
#include <stdio.h>
#include <stdlib.h>

/* BATCHER TESTS*/

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

    enter_batcher(ta->b, ta->t);
    
    int delay2 = rand() % 5000000;
    usleep(delay2);

    leave_batcher(ta->b);

    return NULL;
}


void batcher_test(void) {

    int nb_threads = 100;

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
        usleep(100000); // sleep for 100 milliseconds (0.1 seconds)
        pthread_create(&threads[i], NULL, enter_batcher_thread, (void*)&args[i]);
    }

    for (int i = 0; i < nb_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    destroy_batcher(b);
    b = NULL;
}


/* MEMORY TESTS */

void memory_test(void) {
    // Memory Initialization
    printf("Initializing memory\n");
    memory* mem = init_memory();
    print_memory(mem);

    // Memory Allocation
    printf("Allocating 4 segments\n");
    int new_index = allocate_segment(mem);
    int new_index2 = allocate_segment(mem);
    int new_index3 = allocate_segment(mem);
    int new_index4 = allocate_segment(mem);
    print_memory(mem);

    // Memory Deallocation 1
    printf("Freeing segment %d\n", new_index);
    free_segment(mem, new_index);
    print_memory(mem);

    // Memory Deallocation 2
    printf("Freeing segment %d\n", new_index2);
    free_segment(mem, new_index2);
    print_memory(mem);

    destroy_memory(mem);
    mem = NULL;
}

int main(void) {
    memory_test();
    return 1;
}
