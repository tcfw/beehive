#ifndef _KERNEL_MM_H
#define _KERNEL_MM_H

#include "unistd.h"
#include <kernel/slub.h>

struct page
{
	union
	{
		slub_cache_entry_t slub;
	};
};

// Init the page allocator
void page_alloc_init(void);

// Init the page allocator
void slub_alloc_init(void);

// Get address of the start of the arena
void *page_start_of_arena(void);

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

// Move buddy allocator
void page_reloc(uintptr_t offset);

#endif