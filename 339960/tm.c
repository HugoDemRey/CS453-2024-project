/**
 * @file   tm.c
 * @author [...]
 *
 * @section LICENSE
 *
 * [...]
 *
 * @section DESCRIPTION
 *
 * Implementation of your own transaction manager.
 * You can completely rewrite this file (and create more files) as you wish.
 * Only the interface (i.e. exported symbols and semantic) must be preserved.
**/

// Requested features
#define _GNU_SOURCE
#define _POSIX_C_SOURCE   200809L
#ifdef __STDC_NO_ATOMICS__
    #error Current C11 compiler does not support atomic operations
#endif

// External headers
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

// Internal headers
#include <tm.h>
#include "macros.h"


/* STRUCTS ARE IN TM.H */

/* MEMORY PART */

memory* init_memory(void) {
    memory* mem = (memory*) malloc(sizeof(memory));
    mem->nb_segments = 0;
    mem->segment_size = 0;

    mem->data = (dual_memory_segment*) malloc(sizeof(dual_memory_segment));
    mem->data->already_accessed = false;
    mem->data->read_only = NULL;
    mem->data->read_write = NULL;
    return mem;
}

void destroy_dual_memory_segment(dual_memory_segment* dual_mem_seg) {
    if (dual_mem_seg == NULL) return;
    free(dual_mem_seg->read_only);
    dual_mem_seg->read_only = NULL;
    free(dual_mem_seg->read_write);
    dual_mem_seg->read_write = NULL;
    free(dual_mem_seg);
    dual_mem_seg = NULL;
}

void destroy_memory(memory* mem) {
    if(mem->data->read_only != NULL) {
        destroy_dual_memory_segment(mem->data->read_only);
    }
    if(mem->data->read_write != NULL) {
        destroy_dual_memory_segment(mem->data->read_write);
    }
    free(mem->data);
    mem->data = NULL;
    free(mem);
    mem = NULL;
}

/* BATCHER PART */
batcher* init_batcher(void) {
    batcher* batcher_ptr = (batcher*) malloc(sizeof(batcher));
    batcher_ptr->count = 0;
    batcher_ptr->remaining = 0;
    batcher_ptr->blocked_threads_head = NULL;
    batcher_ptr->blocked_threads_tail = NULL;
    batcher_ptr->enter_lock = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(batcher_ptr->enter_lock, NULL);

    return batcher_ptr;
}

void destroy_batcher(batcher* batcher) {
    if (batcher == NULL) return;
    blocked_thread* current = batcher->blocked_threads_head;
    blocked_thread* next = NULL;
    while (current != NULL) {
        next = current->next;
        sem_destroy(&current->sem);
        free(current);
        current = next;
    }
    free(batcher->enter_lock);
    batcher->enter_lock = NULL;
    free(batcher);
    batcher = NULL;
}

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
    / *
    // Solution 1
    pthread_mutex_lock(batcher->enter_lock);

    /* Wake up the next thread in the list */
    if (blocked_thread->next == NULL) {
        // Solution 1
        batcher->blocked_threads_tail = NULL;
        blocked_thread = NULL;
        return;
    }
    blocked_thread = NULL;

    // Solution 1
    pthread_mutex_unlock(batcher->enter_lock);
    
    printf("Thread %d is waking up the next thread\n", blocked_thread->id);
    sem_post(&blocked_thread->next->sem);

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


/* STM PART */

/** Create (i.e. allocate + init) a new shared memory region, with one first non-free-able allocated segment of the requested size and alignment.
 * @param size  Size of the first shared segment of memory to allocate (in bytes), must be a positive multiple of the alignment
 * @param align Alignment (in bytes, must be a power of 2) that the shared memory region must support
 * @return Opaque shared memory region handle, 'invalid_shared' on failure
**/
shared_t tm_create(size_t unused(size), size_t unused(align)) {
    
    // TODO: tm_create(size_t, size_t)
    return invalid_shared;
}

/** Destroy (i.e. clean-up + free) a given shared memory region.
 * @param shared Shared memory region to destroy, with no running transaction
**/
void tm_destroy(shared_t unused(shared)) {
    // TODO: tm_destroy(shared_t)
}

/** [thread-safe] Return the start address of the first allocated segment in the shared memory region.
 * @param shared Shared memory region to query
 * @return Start address of the first allocated segment
**/
void* tm_start(shared_t unused(shared)) {
    // TODO: tm_start(shared_t)
    return NULL;
}

/** [thread-safe] Return the size (in bytes) of the first allocated segment of the shared memory region.
 * @param shared Shared memory region to query
 * @return First allocated segment size
**/
size_t tm_size(shared_t unused(shared)) {
    // TODO: tm_size(shared_t)
    return 0;
}

/** [thread-safe] Return the alignment (in bytes) of the memory accesses on the given shared memory region.
 * @param shared Shared memory region to query
 * @return Alignment used globally
**/
size_t tm_align(shared_t unused(shared)) {
    // TODO: tm_align(shared_t)
    return 0;
}

/** [thread-safe] Begin a new transaction on the given shared memory region.
 * @param shared Shared memory region to start a transaction on
 * @param is_ro  Whether the transaction is read-only
 * @return Opaque transaction ID, 'invalid_tx' on failure
**/
tx_t tm_begin(shared_t unused(shared), bool unused(is_ro)) {
    // TODO: tm_begin(shared_t)
    return invalid_tx;
}

/** [thread-safe] End the given transaction.
 * @param shared Shared memory region associated with the transaction
 * @param tx     Transaction to end
 * @return Whether the whole transaction committed
**/
bool tm_end(shared_t unused(shared), tx_t unused(tx)) {
    // TODO: tm_end(shared_t, tx_t)
    return false;
}

/** [thread-safe] Read operation in the given transaction, source in the shared region and target in a private region.
 * @param shared Shared memory region associated with the transaction
 * @param tx     Transaction to use
 * @param source Source start address (in the shared region)
 * @param size   Length to copy (in bytes), must be a positive multiple of the alignment
 * @param target Target start address (in a private region)
 * @return Whether the whole transaction can continue
**/
bool tm_read(shared_t unused(shared), tx_t unused(tx), void const* unused(source), size_t unused(size), void* unused(target)) {
    // TODO: tm_read(shared_t, tx_t, void const*, size_t, void*)
    return false;
}

/** [thread-safe] Write operation in the given transaction, source in a private region and target in the shared region.
 * @param shared Shared memory region associated with the transaction
 * @param tx     Transaction to use
 * @param source Source start address (in a private region)
 * @param size   Length to copy (in bytes), must be a positive multiple of the alignment
 * @param target Target start address (in the shared region)
 * @return Whether the whole transaction can continue
**/
bool tm_write(shared_t unused(shared), tx_t unused(tx), void const* unused(source), size_t unused(size), void* unused(target)) {
    // TODO: tm_write(shared_t, tx_t, void const*, size_t, void*)
    return false;
}

/** [thread-safe] Memory allocation in the given transaction.
 * @param shared Shared memory region associated with the transaction
 * @param tx     Transaction to use
 * @param size   Allocation requested size (in bytes), must be a positive multiple of the alignment
 * @param target Pointer in private memory receiving the address of the first byte of the newly allocated, aligned segment
 * @return Whether the whole transaction can continue (success/nomem), or not (abort_alloc)
**/
alloc_t tm_alloc(shared_t unused(shared), tx_t unused(tx), size_t unused(size), void** unused(target)) {
    // TODO: tm_alloc(shared_t, tx_t, size_t, void**)
    return abort_alloc;
}

/** [thread-safe] Memory freeing in the given transaction.
 * @param shared Shared memory region associated with the transaction
 * @param tx     Transaction to use
 * @param target Address of the first byte of the previously allocated segment to deallocate
 * @return Whether the whole transaction can continue
**/
bool tm_free(shared_t unused(shared), tx_t unused(tx), void* unused(target)) {
    // TODO: tm_free(shared_t, tx_t, void*)
    return false;
}
