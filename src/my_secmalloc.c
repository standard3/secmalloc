#define _GNU_SOURCE
#include "my_secmalloc.private.h"
#include <stdio.h>
#include <alloca.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>

#define PAGE_SIZE 4096

chunk_list_t *heap = NULL;
const size_t heap_size = PAGE_SIZE;
const size_t heap_offset_size = 1e5; // 100 000 pages

/**
 * @brief Initializes the heap for secure memory allocation.
 *
 * This function initializes the heap by allocating a chunk of memory using mmap.
 * It sets the initial state, size, and availability of the heap.
 *
 * @return A pointer to the initialized heap.
 */
chunk_list_t *init_heap()
{
    if (heap != NULL)
        return heap;

    // Allocate memory for the heap
    heap = (chunk_list_t *)mmap(
        NULL,
        heap_size * heap_offset_size, // allows to map into the 100000th page
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANON,
        -1,
        0);

    heap->chunk = NULL;
    heap->next = NULL;

    return heap;
}

/**
 * Allocates a chunk of memory with the specified size.
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

    return chunk;
}

chunk_t *find_free_chunk(size_t size)
{
    (void)size;
    return NULL;
}

void *my_malloc(size_t size)
{
    (void)size;
    return NULL;
}

void my_free(void *ptr)
{
    (void)ptr;
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
