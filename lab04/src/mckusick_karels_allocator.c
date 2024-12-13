#include "allocator.h"

typedef struct FreeBlock {
    struct FreeBlock* next;
} FreeBlock;

#define ALIGN(size, alignment) (((size) + (alignment - 1)) & ~(alignment - 1))

Allocator* allocator_create(void* const memory, const size_t size) {
    if (!memory || size < sizeof(Allocator)) {
        return NULL;
    }

    Allocator* allocator = (Allocator*)memory;
    allocator->memory_start = (char*)memory + sizeof(Allocator);
    allocator->memory_size = size - sizeof(Allocator);
    allocator->free_list = allocator->memory_start;

    FreeBlock* block = (FreeBlock*)allocator->memory_start;
    block->next = NULL;

    return allocator;
}

void allocator_destroy(Allocator* const allocator) {
    allocator->memory_start = NULL;
    allocator->memory_size = 0;
    allocator->free_list = NULL;
}

void* allocator_alloc(Allocator* const allocator, const size_t size) {
    if (!allocator || size == 0) {
        return NULL;
    }

    size_t aligned_size = ALIGN(size, sizeof(void*));
    FreeBlock* prev = NULL;
    FreeBlock* curr = (FreeBlock*)allocator->free_list;

    while (curr) {
        if (aligned_size <= allocator->memory_size) {
            if (prev) {
                prev->next = curr->next;
            } else {
                allocator->free_list = curr->next;
            }
            return curr;
        }

        prev = curr;
        curr = curr->next;
    }

    return NULL;
}

void allocator_free(Allocator* const allocator, void* const memory) {
    if (!allocator || !memory) {
        return;
    }

    FreeBlock* block = (FreeBlock*)memory;
    block->next = (FreeBlock*)allocator->free_list;
    allocator->free_list = block;
}
