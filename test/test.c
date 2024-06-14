#include <string.h>   // strcpy, strncpy
#include <sys/mman.h> // mmap, munmap
#include <time.h>     // time

#include <criterion/criterion.h>

#include "my_secmalloc.private.h"
#include "my_secmalloc.h"
#include "utils.h"

extern int log_fd;
extern chunk_list_t *cl_metadata_head;

void setup(void)
{
    init_logging();
    cr_log_info("Setting up...");
}

/* MMAP */

Test(mmap, simple_mmap)
{
    init_logging();

    void *ptr = mmap(
        NULL,
        4096,
        PROT_READ | PROT_WRITE,
        MAP_ANONYMOUS | MAP_PRIVATE,
        -1,
        0);
    cr_expect(ptr != NULL);

    int res = munmap(ptr, 4096);
    cr_expect(res == 0);
}

/* INITIALIZATION */

Test(initialization, initialize)
{
    void *ptr = init_heap();
    cr_expect(ptr != NULL);
}

/* ALLOCATION */

Test(allocation, basic_allocation)
{
    void *ptr = my_malloc(4096);
    cr_expect(ptr != NULL);
    my_free(ptr);
}

Test(allocation, allocation_and_write)
{

    void *ptr = my_malloc(4096);
    cr_expect(ptr != NULL);
    strncpy(ptr, "Hello", 6);
    cr_expect(strcmp(ptr, "Hello") == 0);
    my_free(ptr);
}

Test(allocation, multiple_allocations)
{
    void *ptr1 = my_malloc(1000);
    void *ptr2 = my_malloc(1000);
    void *ptr3 = my_malloc(1000);
    void *ptr4 = my_malloc(1000);

    cr_expect(ptr1 != NULL);
    cr_expect(ptr2 != NULL);
    cr_expect(ptr3 != NULL);
    cr_expect(ptr4 != NULL);

    my_free(ptr1);
    my_free(ptr2);
    my_free(ptr3);
    my_free(ptr4);
}

Test(allocation, fragmentation)
{
    // Allocate multiple blocks
    void *ptr1 = my_malloc(1000);
    void *ptr2 = my_malloc(4096);
    void *ptr3 = my_malloc(1000);

    cr_expect(ptr1 != NULL);
    cr_expect(ptr2 != NULL);
    cr_expect(ptr3 != NULL);

    // Write strings in those
    strncpy(ptr1, "Hello", 6);
    strncpy(ptr2, "World", 6);
    strncpy(ptr3, "!", 2);

    cr_expect(strcmp(ptr1, "Hello") == 0);
    cr_expect(strcmp(ptr2, "World") == 0);
    cr_expect(strcmp(ptr3, "!") == 0);

    my_free(ptr1);
    my_free(ptr2);
    my_free(ptr3);
}

Test(allocation, reallocate_memory)
{
    void *ptr = my_malloc(100);
    void *new_ptr = my_realloc(ptr, 200);

    cr_expect(new_ptr != NULL);

    my_free(new_ptr);
}

Test(allocation, calloc)
{
    void *ptr = my_calloc(10, 20);
    cr_expect(ptr != NULL);

    for (size_t i = 0; i < 10 * 20; i++)
        cr_expect(((char *)ptr)[i] == 0);

    my_free(ptr);
}

// Stress tests
Test(allocation, random_allocations)
{
    srand(time(NULL));

    int num_allocs = rand() % 100 + 1;
    void **ptrs = my_malloc(num_allocs * sizeof(void *));

    // Allocate memory blocks of random sizes
    for (int i = 0; i < num_allocs; i++)
    {
        int size = rand() % 4096 + 1;

        LOG_INFO("Allocating %d bytes", size);
        ptrs[i] = my_malloc(size);
        LOG_INFO("NOT CRASHED");

        cr_expect(ptrs[i] != NULL);
    }

    // Free all the allocated memory
    for (int i = 0; i < num_allocs; i++)
        my_free(ptrs[i]);

    my_free(ptrs);
}

Test(allocation, random_allocations_with_frees)
{
    srand(time(NULL));

    int num_allocs = rand() % 100 + 1;
    void **ptrs = malloc(num_allocs * sizeof(void *));
    bool *freed = malloc(num_allocs * sizeof(bool));

    for (int i = 0; i < num_allocs; i++)
    {
        int size = rand() % 4096 + 1;
        ptrs[i] = my_malloc(size);
        freed[i] = false;

        cr_expect(ptrs[i] != NULL);

        if (rand() % 2 == 0 && i > 0)
        {
            int j;
            do
            {
                j = rand() % i;
            } while (freed[j]);

            my_free(ptrs[j]);
            freed[j] = true;
        }
    }
    for (int i = 0; i < num_allocs; i++)
    {
        if (!freed[i])
            my_free(ptrs[i]);
    }
    free(ptrs);
    free(freed);
}

/* SECURITY FEATURES */

Test(security, buffer_overflow_detection)
{
    void *ptr = my_malloc(100);
    cr_expect(ptr != NULL);

    char *data = (char *)ptr;
    for (int i = 0; i < 150; i++)
        data[i] = 'A'; // Write past the allocated memory

    // Ideally, here we should have a way to detect the overflow
    my_free(ptr);
}

Test(security, double_free_detection)
{
    void *ptr = my_malloc(100);
    cr_expect(ptr != NULL);

    my_free(ptr);

    // Double free
    my_free(ptr);

    // Ideally, here we should have a way to detect the double free
}

Test(security, use_after_free_detection)
{
    void *ptr = my_malloc(100);
    cr_expect(ptr != NULL);

    my_free(ptr);

    char *data = (char *)ptr;
    strcpy(data, "Use after free");

    // Ideally, here we should have a way to detect the use after free
}

Test(security, memory_leak_detection)
{
    void *ptr = my_malloc(100);
    cr_expect(ptr != NULL);

    // Memory leak
}

/* CHUNK LIST */

Test(chunk_list, find_free_block)
{
    init_heap();

    void *ptr1 = my_malloc(100);
    void *ptr2 = my_malloc(200);

    my_free(ptr1);

    chunk_list_t *empty_block = find_free_chunk(100);

    cr_expect(empty_block != NULL);
    cr_expect(empty_block->state == FREE);
    cr_expect(empty_block->size >= 100);

    my_free(ptr2);
}
