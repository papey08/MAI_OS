#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <stdlib.h>

#ifndef BLOCK_COUNT
#define BLOCK_COUNT 128
#endif

typedef struct Allocator {
    uint32_t *memory;
    size_t size;
} Allocator;

Allocator *allocator_create(void *const memory, const size_t size) {
    if (memory == NULL || size < sizeof(Allocator))
        return NULL;
    Allocator *all = (Allocator *)memory;
    all->size = BLOCK_COUNT * 4; // 128 4 byte blocks
    if (all->size % 2 == 1)
        all->size++;

    all->memory = mmap(NULL, all->size, PROT_READ | PROT_WRITE,
                       MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (all->memory == MAP_FAILED)
        return NULL;
    *all->memory = all->size;
    *(all->memory + all->size - 1) = all->size;
    return all;
}

void allocator_destroy(Allocator *const allocator) {
    munmap(allocator->memory, allocator->size);
}

void *allocator_alloc(Allocator *const allocator, const size_t size) {
    size_t size_in_blocks = (size + 3) / 4;
    size_t full_size = size_in_blocks + 2;
    uint32_t *end = allocator->memory + allocator->size;

    size_t diff = -1;
    uint32_t *ptr = NULL;

    for (uint32_t *p = allocator->memory; p < end; p = p + (*p & ~1)) {
        if (*p & 1)
            continue;
        if (*p < full_size) // we need memory for headers
            continue;

        if (size - *p < diff) {
            diff = size - *p;
            ptr = p;
        }
    }
    if (!ptr)
        return NULL;

    size_t newsize =
        full_size % 2 == 0 ? full_size : full_size + 1; // round to even bytes

    size_t oldsize = *ptr & ~1;
    *ptr = newsize | 1;          // front header
    *(ptr + newsize - 1) = *ptr; // back header

    if (newsize < oldsize)
        *(ptr + newsize) = oldsize - newsize;
    return ptr + 1;
}

void allocator_free(Allocator *const allocator, void *const memory) {
    (void)allocator;

    uint32_t *ptr = (uint32_t *)memory - 1; // current pointer
    *ptr = *ptr & ~1;

    uint32_t *front = ptr;
    uint32_t *back = ptr + *ptr - 1;
    size_t size = *ptr;
    printf("size = %zu\n", size);

    uint32_t *next = ptr + *ptr;
    if ((*next & 1) == 0) {
        back += *next;
        size += *next;
    }

    uint32_t *prev_back = ptr - 1;
    if ((*prev_back & 1) == 0) {
        front -= *prev_back;
        size += *prev_back;
    }
    *front = size;
    *back = size;
}
