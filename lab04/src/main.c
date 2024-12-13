#include <stdio.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include "allocator.h"

#define MEMORY_SIZE 1024 * 1024

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <path_to_allocator_library>\n", argv[0]);
        return 1;
    }

    void* handle = dlopen(argv[1], RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "Failed to load library: %s\n", dlerror());
        return 1;
    }

    Allocator* (*allocator_create)(void*, size_t) = dlsym(handle, "allocator_create");
    void (*allocator_destroy)(Allocator*) = dlsym(handle, "allocator_destroy");
    void* (*allocator_alloc)(Allocator*, size_t) = dlsym(handle, "allocator_alloc");
    void (*allocator_free)(Allocator*, void*) = dlsym(handle, "allocator_free");

    char* error;
    if ((error = dlerror()) != NULL) {
        fprintf(stderr, "Error resolving symbols: %s\n", error);
        dlclose(handle);
        return 1;
    }

    void* memory = mmap(NULL, MEMORY_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (memory == MAP_FAILED) {
        perror("mmap failed");
        dlclose(handle);
        return 1;
    }

    Allocator* allocator = allocator_create(memory, MEMORY_SIZE);
    if (!allocator) {
        fprintf(stderr, "Failed to create allocator\n");
        munmap(memory, MEMORY_SIZE);
        dlclose(handle);
        return 1;
    }

    void* block = allocator_alloc(allocator, 128);
    if (block) {
        printf("Allocated block at %p\n", block);
    } else {
        printf("Failed to allocate block\n");
    }

    allocator_free(allocator, block);
    printf("Freed block at %p\n", block);

    allocator_destroy(allocator);
    munmap(memory, MEMORY_SIZE);
    dlclose(handle);

    return 0;
}
