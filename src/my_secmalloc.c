#define _GNU_SOURCE
#include "my_secmalloc.private.h"
#include <stdio.h>
#include <stdlib.h>
#include <alloca.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>

#define PAGE_SIZE 4096

extern int log_fd; // Defined in utils.c, used for logging

chunk_list_t *cl_metadata = NULL;
const size_t heap_size = PAGE_SIZE;
const size_t metadata_offset = 1e5; // 100 000 pages
const size_t metadata_size = heap_size * metadata_offset;

/**
 * @brief Initializes the metadata pool for secure memory allocation.
 *
 * Initializes the metadata pool by allocating a chunk of memory using mmap.
 * The metadata pool is used to store metadata about the allocated chunks, including their address,
 * state, and size.
 *
 * @return A pointer to the initialized metadata pool.
 */
chunk_list_t *init_metadata_pool()
{
    chunk_list_t *metadata_pool = (chunk_list_t *)mmap(
        NULL,
        metadata_size,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANON,
        -1,
        0);

    if (metadata_pool == MAP_FAILED)
        return NULL;

    return metadata_pool;
}

/**
 * @brief Initializes the heaps for secure memory allocation.
 *
 * Tnitializes two pools of memory by allocating chunks of memory using mmap.
 * It sets their initial state, size, and availability.
 * The metadata heap is used to store metadata about the allocated chunks, while the data heap
 * is used to store the actual data.
 *
 * @return A pointer to the initialized metadata heap.
 */
chunk_list_t *init_heap()
{
    // Init logging, file descriptor will be closed at exit
    init_logging();
    atexit(close_logging);

    if (cl_metadata != NULL)
        return cl_metadata;

    log_general(log_fd, LOG_INFO, "init_heap - Initializing pools of memory");

    // Allocate metadata heap
    cl_metadata = init_metadata_pool();
    if (cl_metadata == NULL)
    {
        log_general(log_fd, LOG_ERROR, "init_heap - Failed to allocate metadata pool");
        return NULL;
    }

    // TODO: should be allocated only when needed
    // chunk_t *chunk = init_data_chunk(cl_metadata);
    // if (chunk == MAP_FAILED)
    // {
    //     log_general(log_fd, LOG_ERROR, "init_heap - Failed to allocate data pool");
    //     return NULL;
    // }
    // log_general(log_fd, LOG_INFO, "init_heap - New chunk initialized at address %p", chunk);

    return cl_metadata;
}

/**
 * Allocates a chunk of memory with the specified size.
 *
 * @param size The size of the chunk to allocate.
 * @return A pointer to the allocated chunk, or NULL if allocation fails.
 */
chunk_t *allocate_chunk(size_t size)
{
    log_general(log_fd, LOG_INFO, "allocate_chunk - Allocating chunk of size %zu", size);

    chunk_t *chunk = (chunk_t *)mmap(
        NULL,
        size,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANON,
        -1,
        0);

    if (chunk == MAP_FAILED)
        return NULL;

    chunk->address = (void *)(chunk);
    chunk->state = FREE;
    chunk->size = size;

    log_general(log_fd, LOG_INFO, "allocate_chunk - Chunk allocated at address %p", chunk);

    return chunk;
}

chunk_t *find_free_chunk(size_t size)
{
    (void)size;
    // chunk_list_t *current = heap;
    // while (current != NULL)
    // {
    //     if (current->chunk->state == FREE && current->chunk->size >= size)
    //         return current->chunk;
    //     current = current->next;
    // }

    return NULL;
}

void *my_malloc(size_t size)
{
    (void)size;
    return NULL;
}

void my_free(void *ptr)
{
    if (ptr == NULL)
        return;

    log_general(log_fd, LOG_INFO, "my_free - Freeing chunk at address %p", ptr);

    chunk_t *chunk = (chunk_t *)((chunk_t *)ptr - sizeof(chunk_t));
    chunk->state = FREE;

    log_general(log_fd, LOG_INFO, "my_free - Chunk at address %p freed", ptr);

    // TODO: Merge contiguous free chunks
}

void *my_calloc(size_t nmemb, size_t size)
{
    (void)nmemb;
    (void)size;
    return NULL;
}

void *my_realloc(void *ptr, size_t size)
{
    (void)ptr;
    (void)size;
    return NULL;
}

#ifdef DYNAMIC
void *malloc(size_t size)
{
    return my_malloc(size);
}
void free(void *ptr)
{
    my_free(ptr);
}
void *calloc(size_t nmemb, size_t size)
{
    return my_calloc(nmemb, size);
}

void *realloc(void *ptr, size_t size)
{
    return my_realloc(ptr, size);
}
#endif
