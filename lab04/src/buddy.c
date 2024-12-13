#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#ifdef _MSC_VER
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

typedef struct BuddyNode
{
    int size;                // Размер блока
    int free;                // Свободен ли блок
    struct BuddyNode *left;  // Левый "двойник"
    struct BuddyNode *right; // Правый "двойник"
} BuddyNode;

typedef struct Allocator
{
    BuddyNode *root; // Корневой узел
    void *memory;    // Указатель на начало области памяти
    int totalSize;   // Общий размер памяти
    int offset;      // Текущий смещённый указатель для работы с памятью
} Allocator;

int is_power_of_two(unsigned int n)
{
    return (n > 0) && ((n & (n - 1)) == 0);
}

BuddyNode *create_node(Allocator *allocator, int size)
{
    if (allocator->offset + sizeof(BuddyNode) > allocator->totalSize)
    {
        return NULL;
    }

    BuddyNode *node = (BuddyNode *)((char *)allocator->memory + allocator->offset);
    allocator->offset += sizeof(BuddyNode);
    node->size = size;
    node->free = 1;
    node->left = node->right = NULL;
    return node;
}

EXPORT Allocator *allocator_create(void *mem, size_t size)
{
    if (!is_power_of_two(size))
    {
        const char msg[] = "This allocator initialize a power of two\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        return NULL;
    }

    Allocator *allocator = (Allocator *)mem;
    allocator->memory = (char *)mem + sizeof(Allocator);
    allocator->totalSize = size - sizeof(Allocator);
    allocator->offset = 0;

    allocator->root = create_node(allocator, size);
    if (!allocator->root)
    {
        return NULL;
    }

    return allocator;
}

void split_node(Allocator *allocator, BuddyNode *node)
{
    int newSize = node->size / 2;

    node->left = create_node(allocator, newSize);
    node->right = create_node(allocator, newSize);
}

BuddyNode *allocate_recursive(Allocator *allocator, BuddyNode *node, int size)
{
    if (node == NULL || node->size < size || !node->free)
    {
        return NULL;
    }

    if (node->size == size)
    {
        node->free = 0;
        return (void *)node;
    }

    if (node->left == NULL)
    {
        split_node(allocator, node);
    }

    void *allocated = allocate_recursive(allocator, node->left, size);
    if (allocated == NULL)
    {
        allocated = allocate_recursive(allocator, node->right, size);
    }

    node->free = (node->left && node->left->free) || (node->right && node->right->free);
    return allocated;
}

EXPORT void *allocator_alloc(Allocator *allocator, size_t size)
{
    if (allocator == NULL || size <= 0)
    {
        return NULL;
    }

    while (!is_power_of_two(size))
    {
        size++;
    }

    return allocate_recursive(allocator, allocator->root, size);
}

EXPORT void allocator_free(Allocator *allocator, void *ptr)
{
    if (allocator == NULL || ptr == NULL)
    {
        return;
    }
    BuddyNode *node = (BuddyNode *)ptr;
    if (node == NULL)
    {
        return;
    }

    node->free = 1;

    if (node->left != NULL && node->left->free && node->right->free)
    {
        allocator_free(allocator, node->left);
        allocator_free(allocator, node->right);
        node->left = node->right = NULL;
    }
}

EXPORT void allocator_destroy(Allocator *allocator)
{
    if (!allocator)
    {
        return;
    }

    allocator_free(allocator, allocator->root);
    if (munmap((void *)allocator, allocator->totalSize + sizeof(Allocator)) == 1)
    {
        exit(EXIT_FAILURE);
    }
}
