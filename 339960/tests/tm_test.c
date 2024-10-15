#include "tm.h"
#include <stdio.h>
#include <stdlib.h>


int main(void){

    batcher* b = init_batcher();

    blocked_thread* head = malloc(sizeof(blocked_thread));
    blocked_thread* tail = malloc(sizeof(blocked_thread));

    b->blocked_threads_head = head;
    b->blocked_threads_tail = tail;
    
    printf("Batcher count: %d\n", b->count);
    printf("Batcher remaining: %d\n", b->remaining);
    printf("Batcher blocked_threads_head: %p\n", b->blocked_threads_head);
    printf("Batcher blocked_threads_tail: %p\n", b->blocked_threads_tail);
    printf("Batcher enter_lock: %p\n", b->enter_lock);

    destroy_batcher(b);

    printf("Batcher count: %d\n", b->count);
    printf("Batcher remaining: %d\n", b->remaining);
    printf("Batcher blocked_threads_head: %p\n", b->blocked_threads_head);
    printf("Batcher blocked_threads_tail: %p\n", b->blocked_threads_tail);
    printf("Batcher enter_lock: %p\n", b->enter_lock);

    return 1;
}
