#include "unistd.h"

#ifndef _KERNEL_MM_H
#define _KERNEL_MM_H

struct page
{
};

void *kmalloc(size_t size);
void kfree(void *obj);

struct page *alloc_page(size_t order);
void free_page(struct page *page);

#endif