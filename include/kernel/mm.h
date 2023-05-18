#ifndef _KERNEL_MM_H
#define _KERNEL_MM_H

#include "unistd.h"
#include <kernel/vm.h>

struct page
{
};

void page_alloc_init(void);
void *page_alloc(unsigned int order);
void *page_alloc_s(size_t size);
void page_free(void *page);
unsigned int size_to_order(size_t size);

void *kmalloc(size_t size);
void kfree(void *obj);

void vm_init();
void vm_init_table(vm_table *table);
uintptr_t vm_pa_to_va(vm_table *table, uintptr_t pptr);
uintptr_t vm_va_to_pa(vm_table *table, uintptr_t vptr);
void vm_alloc(vm_table *table, uintptr_t vsstart, size_t size);
int vm_map_region(vm_table *table, uintptr_t pstart, uintptr_t pend, uintptr_t vstart, uintptr_t vend);
void vm_unmap_region(vm_table *table, uintptr_t vstart, uintptr_t vend);
void vm_mark_region(vm_table *table, entry_state_e state, uintptr_t page);
void vm_free_table(vm_table *table);
void vm_set_table(vm_table *table);
void vm_clear_caches();

#endif