#include <criterion/criterion.h>
#include <criterion/redirect.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <time.h>

#include "my_secmalloc.private.h"

extern chunk_list_t *cl_metadata;
extern void *cl_data;

extern const size_t heap_size;
extern const size_t metadata_offset;
extern const size_t metadata_size;

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

    // check if address of the data pool is well over the metadata pool
    cr_expect_geq((size_t)cl_data, (size_t)cl_metadata + metadata_size);

    int res = munmap(heap, heap_size * metadata_offset);
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

Test(allocation, allocate_multiple_chunks)
{
    chunk_t *chunk1 = allocate_chunk(3);
    cr_expect_not_null(chunk1);
    cr_expect_eq(chunk1->size, 3);

    chunk_t *chunk2 = allocate_chunk(5);
    cr_expect_not_null(chunk2);
    cr_expect_eq(chunk2->size, 5);

    chunk_t *chunk3 = allocate_chunk(8);
    cr_expect_not_null(chunk3);
    cr_expect_eq(chunk3->size, 8);

    int res1 = munmap(chunk1, 3);
    cr_expect(res1 == 0);

    int res2 = munmap(chunk2, 5);
    cr_expect(res2 == 0);

    int res3 = munmap(chunk3, 8);
    cr_expect(res3 == 0);
}

Test(allocation, allocate_and_free)
{
    chunk_t *chunk = allocate_chunk(3);
    cr_expect_not_null(chunk);
    cr_expect_eq(chunk->size, 3);

    my_free(chunk);

    cr_expect_eq(chunk->state, FREE);
}

Test(allocation, allocate_multiple_chunks_and_free)
{
    chunk_t *chunk1 = allocate_chunk(3);
    cr_expect_not_null(chunk1);
    cr_expect_eq(chunk1->size, 3);

    chunk_t *chunk2 = allocate_chunk(5);
    cr_expect_not_null(chunk2);
    cr_expect_eq(chunk2->size, 5);

    chunk_t *chunk3 = allocate_chunk(8);
    cr_expect_not_null(chunk3);
    cr_expect_eq(chunk3->size, 8);

    my_free(chunk1);
    my_free(chunk2);
    my_free(chunk3);
}

Test(allocation, init_heap_and_append)
{
    // Initialize the heap
    chunk_list_t *heap = init_heap();
    cr_expect_not_null(heap);
    cr_expect_null(heap->chunk);
    cr_expect_null(heap->next);

    // Allocate chunks
    chunk_t *chunk1 = allocate_chunk(3);
    cr_expect_not_null(chunk1);
    cr_expect_eq(chunk1->size, 3);

    chunk_t *chunk2 = allocate_chunk(5);
    cr_expect_not_null(chunk2);
    cr_expect_eq(chunk2->size, 5);

    chunk_t *chunk3 = allocate_chunk(8);
    cr_expect_not_null(chunk3);
    cr_expect_eq(chunk3->size, 8);

    // Append the chunks to the heap
    heap->chunk = chunk1;
    heap->next = malloc(sizeof(chunk_list_t));
    cr_expect_not_null(heap->next);

    heap->next->chunk = chunk2;
    heap->next->next = malloc(sizeof(chunk_list_t));
    cr_expect_not_null(heap->next->next);

    heap->next->next->chunk = chunk3;
    heap->next->next->next = NULL;
    cr_expect_null(heap->next->next->next);

    // Free the heap
    my_free(chunk1);
    my_free(chunk2);
    my_free(chunk3);

    // Check that the heap is empty
    // cr_expect_null(heap->chunk);
    // cr_expect_null(heap->next->chunk);
    // cr_expect_null(heap->next->next->chunk);
}
