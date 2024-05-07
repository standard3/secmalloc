# TODO

## Memory allocator

- [x] Create global variables
  - [x] const heap_size
  - [x] ptr to heap initialized to NULL

- [x] Create function `chunk *init_heap()`
  - Initializes the heap at arbitrary address (beyond vsdo section) as it allows to get a lot a free memory that will no be overwritten

- [ ] Create function `chunk *find_free_chunk(size_t s)`
  - Iterate on every chunk in the heap and get the first one being FREE and can contain sufficient size s
  - Returns its address (or NULL if not found)

- [ ] Create function `chunk *find_last_chunk(size_t s)`
  - Iterate on every chunk in the heap and get the last chunk.
  - Returns its address (or NULL if not found)

- [ ] Create function `int remap_heap(size_t s)`
  - Calculate delta, old size, ... see get_free_chunk in examples
  - Returns 0 if ok, else NULL

- [ ] Create function `chunk *get_free_chunk(size_t s)`
  - Try to get a free chunk with `find_free_chunk`, if it works, returns it. Else, we need to `mremap` with encapsulated `remap_heap` and retry getting a free chunk.

- [ ] Create function `void *my_malloc(size_t size)`
  - get free chunk
  - split it
  - get the end of it and put it free
  - mark actual free chunk as taken

- [ ] Create function `void my_free(void *ptr)`
  - check if ptr is not garbage (is in scope in general)
  - merge consecutive blocks

- [ ] Rewrite `free`
- [ ] Rewrite `malloc`
- [ ] Rewrite `calloc`
- [ ] Rewrite `realloc`
