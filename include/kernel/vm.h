#ifndef _KERNEL_VM_H
#define _KERNEL_VM_H

#include "unistd.h"
#include <kernel/paging.h>

void vm_init();
void vm_set_kernel();
void vm_init_table(vm_table *table);
uintptr_t vm_pa_to_va(vm_table *table, uintptr_t pptr);
uintptr_t vm_va_to_pa(vm_table *table, uintptr_t vptr);
void vm_alloc(vm_table *table, uintptr_t vsstart, size_t size);
int vm_map_region(vm_table *table, uintptr_t pstart, uintptr_t vstart, size_t size, uint64_t flags);
void vm_unmap_region(vm_table *table, uintptr_t vstart, size_t size);
void vm_mark_region(vm_table *table, entry_state_e state, uintptr_t page);
void vm_free_table(vm_table *table);
void vm_set_table(vm_table *table);
void vm_clear_caches();
void vm_enable();
vm_table *vm_get_current_table();

#endif