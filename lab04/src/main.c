// Вариант 9: списки свободных блоков (наиболее подходящее) и алгоритм двойников
#include <stdint.h>
#include <dlfcn.h>
#include <unistd.h>

#include <stdio.h>
#include <string.h>

#include <sys/mman.h>

#include "allocator.h"

struct Allocator {
    size_t size;
    uint32_t *memory;
};

static allocator_create_func *allocator_create;
static allocator_destroy_func *allocator_destroy;
static allocator_alloc_func *allocator_alloc;
static allocator_free_func *allocator_free;

Allocator *allocator_create_impl(void *const memory, const size_t size) {
    (void)size;
    (void)memory;
    return NULL;
}

void allocator_destroy_impl(Allocator *const allocator) {
    (void)allocator;
}

void *allocator_alloc_impl(Allocator *const allocator, const size_t size) {
    (void)allocator;
    uint32_t *memory = mmap(NULL, size + 1, PROT_READ | PROT_WRITE,
                            MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (memory == MAP_FAILED) {
        return NULL;
    }
    *memory = size + 1;
    return memory + 1;
}

void allocator_free_impl(Allocator *const allocator, void *const memory) {
    (void)allocator;
    if (memory == NULL)
        return;
    uint32_t *mem = memory;
    mem--;
    munmap(mem, *mem);
}

#define LOAD_FUNCTION(name)                                                    \
    {                                                                          \
        name = (name##_func *)dlsym(library, #name);                           \
        if (allocator_create == NULL) {                                        \
            char message[] =                                                   \
                "WARNING: failed to find " #name " function implementation\n"; \
            write(STDERR_FILENO, message, strlen(message));                    \
            name = name##_impl;                                                \
        }                                                                      \
    }

void load_dynamic(const char *path) {
    void *library = dlopen(path, RTLD_LOCAL | RTLD_NOW);
    if (path && library) {
        LOAD_FUNCTION(allocator_create);
        LOAD_FUNCTION(allocator_destroy);
        LOAD_FUNCTION(allocator_alloc);
        LOAD_FUNCTION(allocator_free);
    } else {
        char message[] = "WARNING: failed to load shared library\n";
        write(STDERR_FILENO, message, strlen(message));
        allocator_create = allocator_create_impl;
        allocator_destroy = allocator_destroy_impl;
        allocator_alloc = allocator_alloc_impl;
        allocator_free = allocator_free_impl;
    }
}

int main(int argc, char *argv[]) {
    (void)argc;
    load_dynamic(argv[1]);
    char mem[1024];
    Allocator *all = allocator_create(mem, sizeof(mem));
    int *a = allocator_alloc(all, 12 * sizeof(int));
    int *b = allocator_alloc(all, 12 * sizeof(int));
    for (int i = 0; i < 12; i++) {
        a[i] = i;
        b[12 - i - 1] = i;
    }
    for (int i = 0; i < 12; i++) {
        printf("a[%d] = %d; b[%d] = %d\n", i, a[i], i, b[i]);
    }
    allocator_free(all, a);
    allocator_free(all, b);
    int *c = allocator_alloc(all, 4 * sizeof(int));
    for (int i = 0; i < 4; i++) {
        printf("c[%d] = %d\n", i, c[i]);
    }
    allocator_destroy(all);
}
