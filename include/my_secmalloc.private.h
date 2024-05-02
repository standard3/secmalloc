#ifndef _SECMALLOC_PRIVATE_H
#define _SECMALLOC_PRIVATE_H

#include "my_secmalloc.h"

typedef enum
{
    FREE,
    USED
} chunk_state_t;

typedef struct chunk_t
{
    size_t size;
    chunk_state_t state;
    chunk_t *next;
} chunk_t;

typedef struct
{
    chunk_t *start;
    size_t available;
} chunk_list_t;

void *
my_malloc(size_t size);
void my_free(void *ptr);
void *my_calloc(size_t nmemb, size_t size);
void *my_realloc(void *ptr, size_t size);

#endif
