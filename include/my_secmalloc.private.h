#ifndef _SECMALLOC_PRIVATE_H
#define _SECMALLOC_PRIVATE_H

#include "my_secmalloc.h"

/** @brief Represents the state of a memory chunk. */
typedef enum
{
    FREE,
    USED
} chunk_state_t;

/**
 * @struct chunk_list_t
 * @brief Represents a node in the chunk list.
 *
 * This struct is used to store information about a chunk in the chunk list.
 * It contains a pointer to the actual chunk data and a pointer to the next chunk in the list.
 */
typedef struct chunk_list_t
{
    struct chunk_list_t *next; // Next element in the list
    size_t size;               // Size of the chunk
    void *data;                // Address of chunk data
    chunk_state_t state;       // State of the chunk
    // unsigned int canary;       // Canary protection
} chunk_list_t;

// Heap initialization
void *init_pool(void *addr, size_t size);
chunk_list_t *init_heap(void);

// Chunks
void merge_consecutive_chunks(void);
void *allocate_chunk(size_t size);
void *split_chunk(chunk_list_t *chunk, size_t size);
void *get_free_chunk(size_t size);
chunk_list_t *find_free_chunk(size_t size);

// Security features
void check_memory_leaks(void);

// Secure memory allocation
void my_free(void *ptr);
void *my_malloc(size_t size);
void *my_calloc(size_t nmemb, size_t size);
void *my_realloc(void *ptr, size_t size);

#endif
