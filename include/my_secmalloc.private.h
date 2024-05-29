#ifndef _SECMALLOC_PRIVATE_H
#define _SECMALLOC_PRIVATE_H

#include "my_secmalloc.h"
#include "utils.h"

/** @brief Represents the state of a memory chunk. */
typedef enum
{
    FREE,
    USED
} chunk_state_t;

/**
 * @struct chunk_t
 * @brief Represents a memory chunk in the secure memory allocator.
 *
 * This struct contains information about a memory chunk, including its address,
 * state, size, and available memory.
 */
typedef struct
{
    void *address;       // Address of the chunk
    chunk_state_t state; // State of the chunk
    size_t size;         // Size of the chunk
} chunk_t;

/**
 * @struct chunk_list_t
 * @brief Represents a node in the chunk list.
 *
 * This struct is used to store information about a chunk in the chunk list.
 * It contains a pointer to the actual chunk data and a pointer to the next chunk in the list.
 */
typedef struct
{
    chunk_t *chunk; // Actual chunk data pointer
    chunk_t *next;  // Next chunk in the list
} chunk_list_t;

// Heap initialization
chunk_list_t *init_metadata_pool(void);
chunk_list_t *init_heap(void);

// Chunks
chunk_t *allocate_chunk(size_t size);
chunk_t *find_free_chunk(size_t s);
chunk_t *find_last_chunk(size_t s);
int remap_heap(size_t s);
chunk_t *get_free_chunk(size_t s);

// Secure memory allocation
void *my_malloc(size_t size);
void my_free(void *ptr);
void *my_calloc(size_t nmemb, size_t size);
void *my_realloc(void *ptr, size_t size);

#endif
