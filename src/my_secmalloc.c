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
void *cl_data = NULL;

const size_t heap_size = PAGE_SIZE;
const size_t metadata_offset = 1e5; // 100 000 pages
const size_t metadata_size = heap_size * metadata_offset;

/**
 * @brief Initializes a memory pool for secure memory allocation.
 *
 * Initializes the pool by allocating a chunk of memory using mmap.
 * The pool is used to store either metadata or data about the allocated chunks.
 *
 * @param addr The address of the pool.
 * @param size The size of the pool.
 *
 * @return A pointer to the initialized pool.
 */
chunk_list_t *init_pool(void *addr, size_t size)
{
    chunk_list_t *metadata_pool = (chunk_list_t *)mmap(
        addr,
        size,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANON,
        -1,
        0);

    if (metadata_pool == MAP_FAILED)
    {
        log_general(log_fd, LOG_ERROR, "init_pool - Failed to allocate pool of size %zu", size);
        return NULL;
    }

    log_general(log_fd, LOG_INFO, "init_pool - Pool initialized at address %p", metadata_pool);

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
    // TODO: call atexit with a clean function that will free all the allocated memory

    if (cl_metadata != NULL)
        return cl_metadata;

    log_general(log_fd, LOG_INFO, "init_heap - Initializing pools of memory");

    // Allocate metadata heap
    cl_metadata = init_pool(NULL, metadata_size);
    if (cl_metadata == NULL)
    {
        log_general(log_fd, LOG_ERROR, "init_heap - Failed to allocate metadata pool");
        return NULL;
    }

    // Allocate one page for the data pool at the end of the metadata pool
    cl_data = init_pool(cl_metadata + metadata_size, heap_size);
    if (cl_data == NULL)
    {
        log_general(log_fd, LOG_ERROR, "init_heap - Failed to allocate data pool");
        return NULL;
    }

    return cl_metadata;
}

/**
 * @brief Allocates a chunk of memory with the specified size.
 *
 * @param size The size of the chunk to allocate.
 * @return A pointer to the allocated chunk, or NULL if allocation fails.
 */
chunk_t *allocate_chunk(size_t size)
{
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

    log_general(log_fd, LOG_INFO, "allocate_chunk - Chunk of size %zu allocated at address %p", size, chunk);

    return chunk;
}

chunk_t *find_free_chunk(size_t size)
{
    if (cl_metadata == NULL)
        cl_metadata = init_heap();

    chunk_list_t *current = cl_metadata;
    while (current != NULL)
    {
        if (current->chunk->state == FREE && current->chunk->size >= size)
        {
            log_general(log_fd, LOG_INFO, "find_free_chunk - Found free chunk of size %zu at address %p", size, current->chunk);
            return current->chunk;
        }

        current = current->next;
    }

    return NULL;
}

/**
 * @brief Set the state of a chunk to FREE.
 *
 * @param current The chunk to free.
 */
void free_chunk(chunk_list_t *current)
{
    current->chunk->state = FREE;
}

void *my_malloc(size_t size)
{
    (void)size;
    return NULL;
}

/**
 * @brief Frees a previously allocated chunk of memory.
 *
 * Frees the chunk by setting its state to FREE and merging it with adjacent free chunks.
 *
 * @param ptr A pointer to the chunk to free.
 */
void my_free(void *ptr)
{
    if (ptr == NULL)
        return;

    // Check if the chunk is already free
    chunk_t *chunk = (chunk_t *)((chunk_t *)ptr);
    if (chunk->state == FREE)
    {
        log_general(log_fd, LOG_INFO, "my_free - Chunk at address %p is already free", ptr);
        return;
    }

    // Check if metadata list is initialized
    if (cl_metadata == NULL)
    {
        log_general(log_fd, LOG_ERROR, "my_free - Metadata list is NULL");
        return;
    }

    // Find the chunk in the metadata list
    chunk_list_t *current = cl_metadata;
    do
    {
        log_general(log_fd, LOG_INFO, "my_free - current chunk address: %p", current->chunk);

        // Set chunk state to free
        if (current->chunk == chunk)
        {
            log_general(log_fd, LOG_INFO, "my_free - went here 2");
            free_chunk(current);
            break;
        }

        log_general(log_fd, LOG_INFO, "my_free - went here 3");
        current = current->next;
    } while (current->chunk != chunk);

    log_general(log_fd, LOG_INFO, "my_free - Freed chunk at address %p", chunk);

    // Merge free chunks
    // FIXME: this will not work for non-consecutive metadata "chunks"
    // as elements in the metadata list are not necessarily contiguous
    while (current->chunk->state == FREE)
    {
        // TODO: write a warning if the chunk is not in the metadata list

        chunk_list_t *next = current->next;
        if (next != NULL && next->chunk->state == FREE)
        {
            current->chunk->size += next->chunk->size;
            current->next = next->next;

            log_general(log_fd, LOG_INFO, "my_free - Merged chunk at address %p with chunk at address %p, size was %zu", current->chunk, next->chunk, current->chunk->size);
        }
        current = current->next;
    }

    log_general(log_fd, LOG_INFO, "my_free - Chunk at address %p freed", ptr);
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
