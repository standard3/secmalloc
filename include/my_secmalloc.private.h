#ifndef _SECMALLOC_PRIVATE_H
#define _SECMALLOC_PRIVATE_H

#include "my_secmalloc.h"
#include "utils.h"

typedef enum
{
    FREE,
    USED
} chunk_state_t;

typedef struct
{
    void *address;       // Address of the chunk
    chunk_state_t state; // State of the chunk
    size_t size;         // Size of the chunk
    size_t available;    // Available memory in the chunk
} chunk_t;

typedef struct
{
    chunk_t *chunk; // Actual chunk data pointer
    chunk_t *next;  // Next chunk in the list
} chunk_list_t;

void *my_malloc(size_t size);
void my_free(void *ptr);
void *my_calloc(size_t nmemb, size_t size);
void *my_realloc(void *ptr, size_t size);

#endif
