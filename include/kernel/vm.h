#ifndef _KERNEL_VM_H
#define _KERNEL_VM_H

#include <kernel/paging.h>
#include <kernel/list.h>
#include <kernel/thread.h>
#include <kernel/unistd.h>

#define MEMORY_TYPE_DEVICE (1 << 0)
#define MEMORY_TYPE_KERNEL (1 << 1)
#define MEMORY_TYPE_USER (1 << 2)
#define MEMORY_PERM_RO (1 << 3)
#define MEMORY_NON_EXEC (1 << 4)
#define MEMORY_USER_NON_EXEC (1 << 5)

// Init virtual memory
void vm_init();

// Post init handler after kernel has relocated
// should only be executed by a single core
void vm_init_post_enable();

// Set the kernel page table into the active page table
void vm_set_kernel();

// Get the kernel page table
vm_table *vm_get_kernel();

// Init a page table
void vm_init_table(vm_table *table);

// Convert a physical address to a virtual address
uintptr_t vm_pa_to_va(vm_table *table, uintptr_t pptr);

// Convert a virtual address to a physical address
uintptr_t vm_va_to_pa(vm_table *table, uintptr_t vptr);

// Allocate a set of pages directly into a table for a given size
void vm_alloc(vm_table *table, uintptr_t vsstart, size_t size);

// Map a region of physical memory into the given table
int vm_map_region(vm_table *table, uintptr_t pstart, uintptr_t vstart, size_t size, uint64_t flags);

// Unmap a region of physical memory from the given table
int vm_unmap_region(vm_table *table, uintptr_t vstart, size_t size);

// Mark a region of memory with a given state
int vm_mark_region(vm_table *table, entry_state_e state, uintptr_t page);

// Free the given table from alloc memory
void vm_free_table(vm_table *table);

// Set the given table as the active page table, updating/clearing any
// required caches
void vm_set_table(vm_table *table, pid_t pid);

// Symbolically map all virtual addresses from table 2 onto table 1
int vm_link_tables(vm_table *table1, vm_table *table2);

// Clear any virtual memory caches for the local core
void vm_clear_caches();

// Enable virtual memory mapping for the current core
void vm_enable();

// Get the current active page table
vm_table *vm_get_current_table();

enum AccessType
{
	ACCESS_TYPE_READ = 0,
	ACCESS_TYPE_WRITE = 1,
};

// Checks if the user space is allowed by the current task
int access_ok(enum AccessType type, void *addr, size_t n);

#endif