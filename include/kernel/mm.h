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

void page_alloc_init(void);
void *page_alloc(unsigned int order);
void *page_alloc_s(size_t size);
void page_free(void *page);
unsigned int size_to_order(size_t size);

void *kmalloc(size_t size);
void kfree(void *obj);

#endif