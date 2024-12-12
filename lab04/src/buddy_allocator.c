#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/mman.h>

#include "allocator.h"

#ifndef BLOCK_COUNT
#define BLOCK_COUNT 128
#endif

typedef struct Block Block;
struct Block {
    bool taken;
    uint32_t path;
    uint8_t size_class;
    Block *next;
    Block *prev;
};

#define MIN_SIZE_CLASS                                                         \
    ((size_t)ceil(log2(sizeof(Block) / sizeof(uint32_t) + 1)))
#define MAX_SIZE_CLASS ((size_t)log2(BLOCK_COUNT))

struct Allocator {
    size_t size;
    char *mem;
    Block *free;
};

Block *buddy(Block *p) {
    uint32_t *ptr = (uint32_t *)p;
    if (p->path & (1 << p->size_class))
        ptr -= (1 << p->size_class);
    else
        ptr += (1 << p->size_class);
    return (Block *)ptr;
}

Allocator *allocator_create(void *const memory, const size_t size) {
    if (memory == NULL || size < sizeof(Allocator))
        return NULL;
    Allocator *all = (Allocator *)memory;
    all->size = 1 << MAX_SIZE_CLASS; // 128 4 byte blocks
    all->mem = mmap(NULL, all->size * sizeof(uint32_t), PROT_READ | PROT_WRITE,
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (all->mem == MAP_FAILED)
        return NULL;

    all->free = (Block *)all->mem;
    all->free->taken = false;
    all->free->size_class = MAX_SIZE_CLASS;
    all->free->next = NULL;
    all->free->prev = NULL;
    all->free->path = 0;
    return all;
}

void allocator_destroy(Allocator *const allocator) {
    munmap(allocator->free, allocator->size * sizeof(uint32_t));
}

void *allocator_alloc(Allocator *const all, const size_t size) {
    if (all == 0 || size == 0)
        return NULL;
    size_t size_in_u32 = (size + sizeof(Block) + 3) / 4;
    size_t size_class = ceil(log2(size_in_u32));
    if (size_class > MAX_SIZE_CLASS)
        return NULL;
    Block *p = all->free;
    for (Block *c = all->free; c; c = c->next) {

        if (c->taken || (uint8_t)c->size_class < size_class)
            continue;
        if (p->taken) {
            p = c;
            continue;
        }
        if (c->size_class < p->size_class)
            p = c;
    }
    if (p == NULL || p->taken)
        return NULL;

    if (all->free == p)
        all->free = p->next;
    if (p->next)
        p->next->prev = p->prev;
    if (p->prev)
        p->prev->next = p->next;

    while (p->size_class > size_class) {
        p->size_class = p->size_class - 1;
        Block *other = buddy(p);
        other->size_class = p->size_class;
        other->next = all->free;
        other->prev = NULL;
        other->taken = false;
        other->path = p->path | 1 << p->size_class;
        if (all->free)
            all->free->prev = other;
        all->free = other;
    }
    p->taken = true;
    return (char *)p + sizeof(Block);
}

void allocator_free(Allocator *const allocator, void *const memory) {
    if (!allocator || !memory)
        return;
    Block *p = (Block *)((char *)memory - sizeof(Block));
    while (p->size_class < MAX_SIZE_CLASS) {
        Block *bud = buddy(p);
        if (bud->taken)
            break;
        if (bud->prev)
            bud->prev->next = bud->next;
        if (bud->next)
            bud->next->prev = bud->prev;
        if (allocator->free == bud)
            allocator->free = bud->next;
        // if buddy is free we can remove him from the free list and combine
        // with current
        p = p->path & (1 << p->size_class) ? bud : p;
        p->size_class++;
    }
    p->taken = false;
    p->next = allocator->free;
    p->prev = NULL;
    allocator->free = p;
}
