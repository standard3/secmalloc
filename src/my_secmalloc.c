#define _GNU_SOURCE
#include "my_secmalloc.private.h"
#include <stdio.h>
#include <alloca.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>

#define PAGE_SIZE 4096

// Global variables, serves as "static"
chunk_t *heap = NULL;
const size_t heap_size = PAGE_SIZE;

/**
 * @brief Initializes the heap for secure memory allocation.
 *
 * This function initializes the heap by allocating a chunk of memory using mmap.
 * It sets the initial state, size, and availability of the heap.
 *
 * @return A pointer to the initialized heap.
 */
chunk_t *init_heap()
{
    if (heap == NULL)
    {
        heap = (chunk_t *)mmap(
            NULL,
            heap_size,
            PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANON,
            -1,
            0);
        heap->address = heap;
        heap->state = FREE;
        heap->size = heap_size - sizeof(chunk_t);
        heap->available = heap->size;
    }
    return heap;
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
