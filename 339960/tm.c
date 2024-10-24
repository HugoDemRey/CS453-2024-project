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
#include <stdatomic.h>

// Internal headers
#include <tm.h>
#include "macros.h"


/* STRUCTS ARE IN TM.H */

/* MEMORY PART */

void print_memory(memory* mem) {
    if (mem == NULL) {
        printf("NULL\n");
        return;
    }

    printf("\n###### Memory ######\n");
    printf("Number of segments: %d\n", mem->nb_segments);
    printf("Segment size: %d Bytes \n", mem->segment_size);
    printf("Data Total size: %d Bytes\n", mem->nb_segments * mem->segment_size);
    printf("Lock: %p\n", &mem->alloc_lock);

    printf("Data: \n");
    if (mem->data == NULL) {
        printf("    - No Data\n");
        return;
    } else {
        for (int i = 0; i < mem->nb_segments; i++) {
            printf("    Segment n.%d\n", i);
            printf("       - Already accessed: %s\n", mem->data[i].already_accessed ? "true" : "false");
            printf("       - Read only: %d\n", mem->data[i].read_only);
            printf("       - Read write: %d\n", mem->data[i].read_write);
        }
    }
    printf("#####################\n\n");
}

/**
 * @brief Initializes a dual memory segment.
 *
 * This function sets up a dual memory segment by allocating and initializing
 * the necessary resources. It ensures that the memory segment is properly
 * configured for subsequent operations.
 *
 * @return Returns the pointer to the newly created dual memory segment, or NULL if the allocation failed.
 */
dual_memory_segment* init_dual_memory_segment(u_int8_t init_value) {
    dual_memory_segment* dual_mem_seg_ptr = (dual_memory_segment*) malloc(sizeof(dual_memory_segment));
    if (dual_mem_seg_ptr == NULL) {
        fprintf(stderr, "Failed to allocate memory for dual memory segment\n");
        return NULL;
    }

    dual_mem_seg_ptr->already_accessed = false;
    dual_mem_seg_ptr->read_only = init_value;
    dual_mem_seg_ptr->read_write = init_value;
    
    return dual_mem_seg_ptr;
}

memory* init_memory(void) {
    memory* mem = (memory*) malloc(sizeof(memory));
    mem->nb_segments = 1;
    mem->segment_size = sizeof(dual_memory_segment);

    mem->alloc_lock = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
    if (pthread_mutex_init(mem->alloc_lock, NULL) != 0) {
        fprintf(stderr, "Failed to initialize mutex for memory\n");
        free(mem);
        return NULL;
    }

    // This is the first segment that is allocated by default, it should not be deallocated except by tm_destroy().
    mem->data = init_dual_memory_segment(0);

    return mem;
}

/**
 * @brief Adds a dual memory segment to the memory structure.
 *
 * This function adds a dual memory segment to the memory structure by reallocating
 * the necessary resources. It ensures that the data memory region (that should be initialize by tm_create())
 * is properly configured for subsequent operations.
 *
 * @param mem Pointer to the memory structure.
 * @param dms The dual memory segment to be added.
 * @return The index of the newly added dual memory segment, or -1 if the allocation failed.
 */
int add_dual_memory_segment_to_memory(memory* mem, dual_memory_segment dms) {
    if (mem == NULL) return -1;
    // We assume that the allocation of the first segment is already dony by tm_create and should not be deallocated.
    int new_segment_index = mem->nb_segments;
    if (mem->data == NULL || new_segment_index == 0) return -1;

    mem->nb_segments++;
    mem->data = (dual_memory_segment*) realloc(mem->data, mem->segment_size * (mem->nb_segments));
    if (mem->data == NULL) {
        fprintf(stderr, "Failed to reallocate memory for dual memory segment\n");
        return -1;
    }

    mem->data[new_segment_index] = dms;
    return new_segment_index;

}

void destroy_memory(memory* mem) {
    if (mem == NULL) return;
    if (mem->alloc_lock != NULL) {
        pthread_mutex_destroy(mem->alloc_lock);
    } 
    //free(mem->data);
    mem->data = NULL;
    free(mem);
    mem = NULL;
}


/**
 * @brief Implementation of concurrent algorithms for transactional memory.
 * @param mem The memory structure to allocate the segment in.
 * @return The index of the newly allocated segment.
 * This function does not modify the memory itsfelf, it uses other functions to do so.
 * This function uses a mutex to lock the memory structure during the calls to the other functions.
 */
int allocate_segment(memory* mem) {
    if (mem == NULL) return -1;

    dual_memory_segment* dms_ptr = init_dual_memory_segment(0);
    if (dms_ptr == NULL) {
        printf("Failed to allocate memory for dual memory segment\n");
        return -1;
    }

    dual_memory_segment dms = *dms_ptr;

    pthread_mutex_lock(mem->alloc_lock);
    int new_index = add_dual_memory_segment_to_memory(mem, dms);

    pthread_mutex_unlock(mem->alloc_lock);
    return new_index;

}

/**
 * FIXME : See if this implementation is correct.
 * 
 * @brief Deallocates a memory segment.
 *
 * This function is responsible for deallocating a previously allocated
 * memory segment. It ensures that all resources associated with the 
 * segment are properly released to avoid memory leaks.
 *
 * @param segment A pointer to the memory segment to be deallocated.
 * @param index The index of the segment to be deallocated.
 */
void deallocate_segment(memory* mem, int index) {
    if (mem == NULL) {
        fprintf(stderr, "Memory structure is NULL\n");
        return;
    }

    if (index == 0) {
        fprintf(stderr, "Cannot deallocate the first segment. Only tm_create() and tm_destroy() can manage the allocation of this segment.\n");
        return;
    }

    if (index < 1 || index >= mem->nb_segments) {
        fprintf(stderr, "Index out of bounds\n");
        return;
    }

    pthread_mutex_lock(mem->alloc_lock);
    mem->nb_segments--;
    for (int i = index; i < mem->nb_segments; i++) {
        mem->data[i] = mem->data[i+1];
    }  
    mem->data = (dual_memory_segment*) realloc(mem->data, mem->segment_size * (mem->nb_segments));


    pthread_mutex_unlock(mem->alloc_lock);
}

/* BATCHER PART */
void print_blocked_thread(blocked_thread* bt) {
    if(bt == NULL) {
        printf("    - NULL\n");
        return;
    }
    printf("    - ptr: %p\n", bt);
    printf("    - id: %d\n", bt->id);
    printf("    - next: %p\n", bt->next);
    printf("    - sem: %d\n", bt->sem);
}

void print_batcher(batcher* b) {
    printf("\n###### Batcher ######\n");
    if (b == NULL) {
        printf("NULL\n");
        return;
    }

    printf("Count: %d\n", b->epoch);
    printf("Remaining: %d\n", b->remaining);

    printf("Blocked threads HEAD: \n");
    print_blocked_thread(b->blocked_threads_head);
    printf("Blocked threads TAIL: \n");
    print_blocked_thread(b->blocked_threads_tail);

    printf("Blocked threads: \n");
    blocked_thread* bt = b->blocked_threads_head;
    while (bt != NULL) {
        printf("    - thread id n.%d \n", bt->id);
        bt = bt->next;
    }
    printf("#####################\n\n");
}

batcher* init_batcher(void) {
    batcher* batcher_ptr = (batcher*) malloc(sizeof(batcher));
    if (batcher_ptr == NULL) {
        fprintf(stderr, "Failed to allocate memory for batcher\n");
        return NULL;
    }

    batcher_ptr->epoch = 0;
    batcher_ptr->remaining = 0;
    batcher_ptr->blocked_threads_head = NULL;
    batcher_ptr->blocked_threads_tail = NULL;

    batcher_ptr->lock = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
    if (batcher_ptr->lock == NULL) {
        fprintf(stderr, "Failed to allocate memory for mutex\n");
        free(batcher_ptr);
        return NULL;
    }

    if (pthread_mutex_init(batcher_ptr->lock, NULL) != 0) {
        fprintf(stderr, "Failed to initialize mutex for batcher\n");
        free(batcher_ptr->lock);
        free(batcher_ptr);
        return NULL;
    }

    return batcher_ptr;
}

void destroy_batcher(batcher* batcher) {
    if (batcher == NULL) return;

    blocked_thread* current = batcher->blocked_threads_head;
    while (current != NULL) {
        blocked_thread* next = current->next;
        sem_destroy(&current->sem);
        free(current);
        current = next;
    }

    if (batcher->lock != NULL) {
        pthread_mutex_destroy(batcher->lock);
        batcher->lock = NULL;
    }

    free(batcher);
}

void wake_up_threads(batcher* batcher) {
    if (batcher->blocked_threads_head == NULL) return;
    sem_post(&batcher->blocked_threads_head->sem);
}

void enter_batcher(batcher* batcher, blocked_thread* blocked_thread) {


    pthread_mutex_lock(batcher->lock);


    if (batcher->remaining == 0) {
        // Maybe use atomic operations of C
        batcher->remaining++;

        pthread_mutex_unlock(batcher->lock);
        printf("🆕 | Thread %d\n\n", blocked_thread->id);

        //print_batcher(batcher);
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

    pthread_mutex_unlock(batcher->lock);

    
    /* Make the current thread sleep using sem (sem_t) which is contained in blocked_thread, wake him up when sem tells the thread to wake up */
    //print_batcher(batcher);
    printf("⌛ | Thread %d\n\n", blocked_thread->id);
    sem_wait(&blocked_thread->sem);
    printf("🆕 | Thread %d\n\n", blocked_thread->id);



    /* 
    FIXME : If a thread assigns itself right after the IF condition as the next element of the tail, it will never be woken up.
    Solution 1: lock the mutex again and check if the next element is the current thread, if so, wake it up.
    */
    // Solution 1
    pthread_mutex_lock(batcher->lock);
    batcher->remaining++;

    //print_batcher(batcher);

    /* Wake up the next thread in the list */
    if (blocked_thread->next == NULL) {
        // Solution 1
        batcher->blocked_threads_tail = NULL;
        batcher->blocked_threads_head = NULL;
        blocked_thread = NULL;

        pthread_mutex_unlock(batcher->lock);

        return;
    }

    // Solution 1
    pthread_mutex_unlock(batcher->lock);
    
    sem_post(&blocked_thread->next->sem);
    blocked_thread = NULL;

}

void leave_batcher(batcher* batcher) {
    pthread_mutex_lock(batcher->lock);

    int remaining = batcher->remaining;

    if (remaining == 1) {
        printf("🚀 | Thread X (remaining=%d) Waking up the Blocked Threads.\n\n", remaining);

        /* Update the memory since no thread is able to access it. I.e. all other threads are sleeping */
        
        /* 
        We would maybe want to use a lock to prevent a new process to enter the batcher and bypass the semaphore by seeing remaining == 0
        but it is not necessary since if a new process arrives at this moment, it will not be blocked and work with the other processes and also increment "remaining" by one as expected.
        */
        batcher->remaining--;
        batcher->epoch++;
        print_batcher(batcher);
        wake_up_threads(batcher);

        //print_batcher(batcher);

        pthread_mutex_unlock(batcher->lock);

        return;
    }
    
    printf("🚀 | Thread X (remaining=%d)\n\n", remaining);
    // Maybe use atomic operations of C
    batcher->remaining--;
    //print_batcher(batcher);
    pthread_mutex_unlock(batcher->lock);
}


/* STM PART */

/** Create (i.e. allocate + init) a new shared memory region, with one first non-free-able allocated segment of the requested size and alignment.
 * @param size  Size of the first shared segment of memory to allocate (in bytes), must be a positive multiple of the alignment
 * @param align Alignment (in bytes, must be a power of 2) that the shared memory region must support
 * @return Opaque shared memory region handle, 'invalid_shared' on failure
**/
shared_t tm_create(size_t unused(size), size_t unused(align)) {
    if (size <= 0 || align <= 0 || size > (1ULL << 48) || align % 2 != 0) return invalid_shared;
    if (size % align != 0) return invalid_shared;
    
    // FIXME: Ask about the alignment of the memory with the structures already created.
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
