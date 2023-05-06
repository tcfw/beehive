#ifndef _KERNEL_MM_H
#define _KERNEL_MM_H

#include "unistd.h"

struct page
{
};

void *kmalloc(size_t size);
void kfree(void *obj);

void page_alloc_init(void);
void *page_alloc(unsigned int order);
void *page_alloc_s(size_t size);
void page_free(void *page);
unsigned int size_to_order(size_t size);

#endif