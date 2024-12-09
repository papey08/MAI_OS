#include "list_allocator.c"
#include <stdio.h>
// Вариант 9: списки свободных блоков (наиболее подходящее) и алгоритм двойников
//
void display_allocation(Allocator *all) {
    printf("[");
    for (size_t i = 0; i < all->size; i++) {
        printf("%u ", all->memory[i]);
    }
    printf("]\n\n");
}

int main(void) {
    char memory[128];
    Allocator *all = allocator_create(memory, sizeof(memory));
    display_allocation(all);
    int *p = allocator_alloc(all, 12 * sizeof(int) + 1);
    for (int i = 0; i < 12; i++) {
        p[i] = i;
    }
    char *c = allocator_alloc(all, 13);
    for (int i = 0; i < 13; i++)
        c[i] = 'a' + i;

    display_allocation(all);
    allocator_free(all, p);
    display_allocation(all);
    allocator_free(all, c);
    display_allocation(all);
    allocator_destroy(all);
    return 0;
}
