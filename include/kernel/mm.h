#ifndef _KERNEL_MM_H
#define _KERNEL_MM_H

#include "unistd.h"

struct page
{
};

void *kmalloc(size_t size);
void kfree(void *obj);

void page_alloc_init(void);
void *alloc_page(unsigned int order);
void *alloc_page_s(size_t size);
void free_page(void *page);
unsigned int size_to_order(size_t size);

#endif