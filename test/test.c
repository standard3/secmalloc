#include <criterion/criterion.h>
#include <criterion/redirect.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <time.h>

#include "my_secmalloc.private.h"

extern const size_t heap_size;
extern const size_t heap_offset_size;

Test(mmap, simple_mmap)
{
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

Test(allocation, init_heap)
{
    chunk_list_t *heap = init_heap();
    cr_expect_not_null(heap);
    cr_expect_null(heap->chunk);
    cr_expect_null(heap->next);

    int res = munmap(heap, heap_size * heap_offset_size);
    cr_expect(res == 0);
}

Test(allocation, allocate_small_chunk)
{
    chunk_t *chunk = allocate_chunk(3);
    cr_expect_not_null(chunk);
    cr_expect_eq(chunk->size, 3);

    int res = munmap(chunk, 3);
    cr_expect(res == 0);
}

Test(allocation, allocate_big_chunk)
{
    chunk_t *chunk = allocate_chunk(heap_size);
    cr_expect_not_null(chunk);
    cr_expect_eq(chunk->size, heap_size);

    int res = munmap(chunk, heap_size);
    cr_expect(res == 0);
}

Test(allocation, allocate_huge_chunk)
{
    const size_t chunk_size = heap_size * 400;

    chunk_t *chunk = allocate_chunk(chunk_size);
    cr_expect_not_null(chunk);
    cr_expect_eq(chunk->size, chunk_size);

    int res = munmap(chunk, chunk_size);
    cr_expect(res == 0);
}
