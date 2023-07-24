#ifndef _KERNEL_MM_H
#define _KERNEL_MM_H

#include "unistd.h"

struct page
{
};

#define MEMORY_TYPE_DEVICE (1 << 0)
#define MEMORY_TYPE_KERNEL (1 << 1)
#define MEMORY_TYPE_USER (1 << 2)
#define MEMORY_PERM_RO (1 << 3)
#define MEMORY_NON_EXEC (1 << 4)
#define MEMORY_USER_NON_EXEC (1 << 5)

// Init the page allocator
void page_alloc_init(void);

// Get a free page at the order size
struct page *page_alloc(unsigned int order);

// Get a free page at a specific size
struct page *page_alloc_s(size_t size);

// Free an allocated page
void page_free(void *page);

// Get the required order which can fit the desired size
unsigned int size_to_order(size_t size);

// Allocate a region for the given sized object
void *kmalloc(size_t size);

// Free a given object
void kfree(void *obj);

#endif