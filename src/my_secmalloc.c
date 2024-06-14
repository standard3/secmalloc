#define _GNU_SOURCE
#include <sys/mman.h> // mmap, munmap
#include <stdarg.h>   // va_list, va_start, va_end
#include <string.h>   // memset, memcpy
#include <stdlib.h>   // atexit

#include "my_secmalloc.private.h"

#define PAGE_SIZE 4096

extern int log_fd; // Defined in utils.c, used for logging

chunk_list_t *cl_metadata_head = NULL;

const size_t metadata_offset = 1e4; // 100 000 pages
unsigned int metadata_size = 0;

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
void *init_pool(void *addr, size_t size)
{
    void *pool = (chunk_list_t *)mmap(
        addr,
        size,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANON,
        -1,
        0);

    if (pool == MAP_FAILED)
    {
        LOG_ERROR("init_pool - Failed to allocate pool of size %zu", size);
        return NULL;
    }

    return pool;
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
    // Don't relaunch if already initialized
    if (cl_metadata_head != NULL)
        return cl_metadata_head;

    // Initialize logging
    init_logging();
    atexit(close_logging);
    atexit(check_memory_leaks);
    atexit(clean);

    LOG_INFO("init_heap - Initializing pools of memory");

    // Allocate our metadata pool
    size_t meta_size = sizeof(chunk_list_t) * metadata_offset;
    void *ptr = init_pool(NULL, meta_size);
    if (ptr == NULL)
    {
        LOG_ERROR("init_heap - Failed to allocate metadata pool");
        munmap(NULL, meta_size);
        return NULL;
    }

    // Allocate a page for our data pool
    void *data_pool = (chunk_list_t *)((chunk_list_t *)ptr + (sizeof(chunk_list_t) * metadata_offset));
    void *ptr_data = init_pool(data_pool, PAGE_SIZE);
    if (ptr_data == NULL)
    {
        LOG_ERROR("init_heap - Failed to allocate data pool");
        munmap(ptr, meta_size);
        return NULL;
    }

    // Set our global variables to our newly created pool
    chunk_list_t *cl_metadata = (chunk_list_t *)ptr;
    metadata_size++;
    cl_metadata->data = ptr_data;
    cl_metadata->size = PAGE_SIZE - sizeof(canary_t);
    cl_metadata->state = FREE;
    cl_metadata->next = NULL;
    set_chunk_canary(cl_metadata);

    cl_metadata_head = cl_metadata;

    return ptr;
}

/**
 * @brief Finds the first free chunk containg at least @size in the metadata list.
 *
 * @param size The size of the chunk to find.
 * @return A pointer to the last chunk in the metadata list or NULL if no free chunk is found.
 */
chunk_list_t *find_free_chunk(size_t size)
{
    chunk_list_t *current = cl_metadata_head;
    while (current != NULL)
    {
        if (current->state == FREE && current->size >= size + sizeof(canary_t))
        {
            LOG_INFO("find_free_chunk - Found free chunk of size %zu at address %p", size, current->data);
            return current;
        }
        current = current->next;
    }

    return NULL;
}

/**
 * @brief Allocates a new chunk of memory with the specified size.
 * This function is used when no free block is found in the memory pool.
 *
 * @param size The size of the memory block to allocate.
 * @return A pointer to the allocated memory block.
 */
void *allocate_chunk(size_t size)
{
    LOG_INFO("allocate_chunk - Allocating chunk of size %zu", size);

    // Create a new metadata entry at the end of the list
    chunk_list_t *new_metadata = (chunk_list_t *)(cl_metadata_head + (sizeof(chunk_list_t) * metadata_size++));

    // Allocate a new chunk of memory
    void *data = init_pool(cl_metadata_head + (sizeof(chunk_list_t) * metadata_offset), size + sizeof(canary_t));
    new_metadata->data = (uint8_t *)(data);
    new_metadata->size = size;
    new_metadata->state = USED;
    new_metadata->next = NULL;
    set_chunk_canary(new_metadata);

    // Split the remaining free space into a new chunk
    // if the size of the allocated block is not a multiple of the page size
    if (size + sizeof(canary_t) % PAGE_SIZE != 0)
    {
        chunk_list_t *empty_next = (chunk_list_t *)(cl_metadata_head + (sizeof(chunk_list_t) * metadata_size++));
        empty_next->data = (uint8_t *)(data) + size + sizeof(canary_t); // + sizeof(canary_t) to avoid canary overwrite
        empty_next->size = PAGE_SIZE - (((size + sizeof(canary_t)) % PAGE_SIZE) + sizeof(canary_t));
        empty_next->state = FREE;
        empty_next->next = NULL;
        set_chunk_canary(empty_next);

        new_metadata->next = empty_next;
    }

    // Append the new metadata entry to the end of the list
    chunk_list_t *current = cl_metadata_head;
    while (current->next != NULL)
        current = current->next;
    current->next = new_metadata;

    return new_metadata->data;
}

/**
 * Split a chunk into two chunks, one for the allocated data and one for the remaining free space.
 * This function is used when the allocated block is empty.
 *
 * @param chunk A pointer to the metadata of the free block.
 * @param size The size of the memory block to allocate.
 * @return A pointer to the allocated memory block.
 */
void *split_chunk(chunk_list_t *chunk, size_t size)
{
    // If the size is too large, we can't split the chunk, so we use it directly
    if (chunk->size + (sizeof(canary_t) * 2) <= size)
    {
        LOG_INFO("split_chunk - chunk size is smaller than the requested size");
        chunk->state = USED;
        return chunk->data;
    }

    // If the size of the free chunk is exactly the same as the requested size, we can use it directly
    chunk_list_t *empty = (chunk_list_t *)(cl_metadata_head + (sizeof(chunk_list_t) * metadata_size++));
    empty->data = (uint8_t *)(chunk->data) + size + sizeof(canary_t); // + sizeof(canary_t) to avoid precedent canary overwrite
    empty->size = chunk->size - (size + sizeof(canary_t));
    empty->state = FREE;
    empty->next = chunk->next;
    set_chunk_canary(empty);

    // Update the metadata of the free chunk
    chunk->size = size;
    chunk->state = USED;
    chunk->next = empty;
    set_chunk_canary(chunk);

    return chunk->data;
}

/**
 * @brief Allocates a chunk of memory with the specified size.
 *
 * @param size The size of the chunk to allocate.
 * @return A pointer to the allocated chunk, or NULL if allocation fails.
 */
void *get_free_chunk(size_t size)
{
    size = (size % 16 ? size + 16 - (size % 16) : size); // Align the size to 16 bytes

    LOG_INFO("get_free_chunk - Allocating chunk of size %zu", size);

    void *data = NULL;
    chunk_list_t *free_chunk = find_free_chunk(size);

    if (free_chunk == NULL)
        // If no free chunk is found, we need to allocate a new chunk
        data = allocate_chunk(size);
    else
        // Divide the free chunk into two chunks, one for the allocated data and one for the remaining free space
        data = split_chunk(free_chunk, size);

    return data;
}

/**
 * @brief Merges consecutive free memory chunks in the memory pool.
 * This function iterates through the memory chunks and merges any adjacent chunks that are both free.
 * It updates the size of the current chunk by adding the size of the next chunk and
 * removes the next chunk from the linked list.
 * This process continues until there are no more adjacent free chunks to merge.
 */
void merge_consecutive_chunks()
{
    chunk_list_t *current = cl_metadata_head;
    while (current != NULL)
    {
        // Current chunk is free, not null and its next chunk is free
        // Ensure the next chunk is on the same page
        chunk_list_t *tmp = current;
        size_t size = tmp->size;

        // Merge consecutive free chunks
        while (tmp->state == FREE && tmp->next != NULL && tmp->next->state == FREE && (uint8_t *)tmp->data + tmp->size + sizeof(canary_t) == tmp->next->data)
        {
            size += tmp->next->size + sizeof(canary_t);
            tmp->canary = tmp->next->canary;
            tmp = tmp->next;
        }

        // Update the current chunk
        current->next = tmp->next;
        current->size = size;
        current->canary = tmp->canary;

        current = current->next;
    }
}

/**
 * @brief Retrieves the metadata structure associated with a given pointer.
 *
 * @param ptr The pointer whose metadata structure needs to be retrieved.
 * @return A pointer to the metadata structure if found, otherwise NULL.
 */
chunk_list_t *get_chunk(void *ptr)
{
    chunk_list_t *current = cl_metadata_head;
    while (current != NULL)
    {
        if (current->data == ptr)
            return current;

        current = current->next;
    }

    return NULL;
}

/**
 * @brief Set a random canary value to a chunk.
 * The canary value is used to detect heap overflows, it is placed at the end of the data block.
 * Is it a random value to make it harder to predict.
 *
 * @return 0 if the canary was set, -1 otherwise.
 */
int set_chunk_canary(chunk_list_t *chunk)
{
    if (chunk == NULL)
        return -1;

    canary_t canary = get_random_canary();
    if (canary == 0)
    {
        LOG_ERROR("set_chunk_canary - can't get a canary");
        return -1;
    }

    chunk->canary = canary;

    memcpy(
        (uint8_t *)(chunk->data) + chunk->size,
        &chunk->canary,
        sizeof(canary_t));

    return 0;
}

/**
 * @brief Check the integrity of the canary value of a chunk.
 * The canary value is used to detect heap overflows, it is placed at the end of the data block.
 *
 * @param chunk The chunk to check the canary value.
 */
void check_canary_integrity(chunk_list_t *chunk)
{
    canary_t canary = 0;
    memcpy(
        &canary,
        (uint8_t *)(chunk->data) + chunk->size,
        sizeof(canary_t));

    if (canary != chunk->canary)
        LOG_ERROR("check_canary_integrity - canary corrupted");

    return;
}

/**
 * @brief Frees a previously allocated memory block.
 *
 * This function marks the memory block pointed to by `ptr` as free. If the block is already free,
 * an error message is logged. After marking the block as free, the function may perform block merging
 * to optimize memory usage.
 *
 * @param ptr A pointer to the memory block to be freed.
 */
void my_free(void *ptr)
{
    if (ptr == NULL)
    {
        LOG_WARN("my_free - null pointer given");
        return;
    }

    chunk_list_t *chunk = get_chunk(ptr);
    if (chunk == NULL)
    {
        LOG_WARN("my_free - chunk not found");
        return;
    }

    // Check double free
    if (chunk->state == FREE)
    {
        LOG_WARN("my_free - double free");
        return;
    }

    // Check canary integrity
    check_canary_integrity(chunk);

    // Free the chunk
    if (chunk->state == USED)
        chunk->state = FREE;

    merge_consecutive_chunks();
}

/**
 * @brief Allocates a block of memory of the given size using a secure memory allocation mechanism.
 *
 * @param size The size of the memory block to allocate.
 * @return A pointer to the allocated memory block, or NULL if the
 * allocation fails.
 */
void *my_malloc(size_t size)
{
    // If the metadata pointer is NULL, we must initialize our heap
    if (cl_metadata_head == NULL)
    {
        void *ptr = init_heap();
        if (ptr == NULL)
        {
            LOG_ERROR("my_malloc - can't initialize heap");
            return NULL;
        }
    }

    // Check if the size is less than or equal to 0
    if (size <= 0)
        return NULL; // FIXME: should return a freeable chunk

    // Allocate data block
    void *ptr_data = get_free_chunk(size);
    if (ptr_data == NULL)
    {
        LOG_ERROR("my_malloc - can't allocate chunk of size %zu", size);
        return NULL;
    }

    return ptr_data;
}

/**
 * @brief Reallocates a memory block with a new size.
 *
 * @param ptr   Pointer to the memory block to be reallocated.
 * @param size  New size for the memory block.
 * @return      Pointer to the reallocated memory block, or NULL if the reallocation failed.
 */
void *my_realloc(void *ptr, size_t size)
{
    // If size is 0, equals to my_free(ptr) and returning NULL
    if (size == 0)
    {
        LOG_INFO("my_realloc - null size given, freeing chunk at %p", ptr);

        my_free(ptr);
        return NULL;
    }

    // If ptr is NULL, equals to my_malloc(size)
    if (ptr == NULL)
    {
        LOG_INFO("my_realloc - null pointer given, allocating a new chunk");
        return my_malloc(size);
    }

    // Retrieve the associated chunk from its address
    chunk_list_t *chunk = get_chunk(ptr);

    // If the chunk is not found, return NULL
    if (chunk == NULL)
        return NULL;

    // If the current size of the memory block is already greater or equal to the new size,
    // return the original pointer
    if (chunk->size >= size)
        return ptr;

    // Allocate a new memory block with the new size
    void *new = my_malloc(size);

    // If the allocation failed, return NULL
    if (new == NULL)
    {
        LOG_WARN("my_realloc - allocation failed");
        return NULL;
    }

    // Copy the contents of the original memory block to the new memory block
    memcpy(new, ptr, chunk->size);

    // Free the original memory block
    my_free(ptr);

    // Return the pointer to the new memory block
    return new;
}

/**
 * @brief Allocates memory for an array of elements and initializes them to zero.
 *
 * This function allocates memory for an array of `nmemb` elements, each of size `size`,
 * and initializes all the elements to zero. It is similar to the standard `calloc` function.
 *
 * @param nmemb The number of elements to allocate memory for.
 * @param size The size of each element in bytes.
 * @return A pointer to the allocated memory, or `NULL` if the allocation fails.
 */
void *my_calloc(size_t nmemb, size_t size)
{
    void *ptr = my_malloc(nmemb * size);
    if (ptr == NULL)
    {
        LOG_ERROR("my_calloc - Can't get a chunk");
        return NULL;
    }
    memset(ptr, 0, nmemb * size);

    return ptr;
}

/**
 * @brief Verifies if all allocated memory blocks have been freed and logs any leaks.
 *
 * Free any lasting allocated chunk.
 */
void check_memory_leaks()
{
    chunk_list_t *current = cl_metadata_head;
    while (current != NULL)
    {
        if (current->state == FREE)
        {
            my_free(current);
            LOG_WARN("Freed non-freed chunk at %p with size %zu", current->data, current->size);
        }
        current = current->next;
    }
}

/**
 * @brief Cleans up the memory pool.
 */
void clean()
{
    // Get total size of the data pool
    chunk_list_t *current = cl_metadata_head;
    while (current != NULL)
    {
        munmap(current->data, current->size + sizeof(canary_t));
        current = current->next;
    }

    // Free the metadata pool
    munmap(cl_metadata_head, metadata_offset * sizeof(chunk_list_t));

    // Reset the global variables
    cl_metadata_head = NULL;
    metadata_size = 0;

    LOG_INFO("clean - Memory pool cleaned");
}

#ifdef DYNAMIC

/**
 * Custom implementation of the malloc function.
 * Allocates a block of memory of the given size.
 *
 * @param size The size of the memory block to allocate.
 * @return A pointer to the allocated memory block, or NULL if the allocation fails.
 */
void *malloc(size_t size)
{
    return my_malloc(size);
}

/**
 * Custom implementation of the free function.
 * Frees the memory block pointed to by the given pointer.
 *
 * @param ptr A pointer to the memory block to free.
 */
void free(void *ptr)
{
    my_free(ptr);
}

/**
 * Custom implementation of the calloc function.
 * Allocates a block of memory for an array of nmemb elements, each of size bytes,
 * and initializes all bytes to zero.
 *
 * @param nmemb The number of elements in the array.
 * @param size The size of each element in bytes.
 * @return A pointer to the allocated memory block, or NULL if the allocation fails.
 */
void *calloc(size_t nmemb, size_t size)
{
    return my_calloc(nmemb, size);
}

/**
 * Custom implementation of the realloc function.
 * Changes the size of the memory block pointed to by the given pointer to the given size.
 *
 * @param ptr A pointer to the memory block to reallocate.
 * @param size The new size of the memory block.
 * @return A pointer to the reallocated memory block, or NULL if the reallocation fails.
 */
void *realloc(void *ptr, size_t size)
{
    return my_realloc(ptr, size);
}

#endif
