#include <kernel/buddy.h>
#include <kernel/mm.h>
#include <kernel/vm.h>
#include <kernel/tty.h>
#include <kernel/strings.h>
#include <kernel/sync.h>
#include <kernel/arch.h>
#include "stdint.h"

extern uint64_t kernelend;

struct buddy_t *pages;
static spinlock_t page_lock;

void page_alloc_init()
{
	spinlock_init(&page_lock);

	uintptr_t ram_max_addr = (uintptr_t)ram_max();
	uint64_t buddy_struct_size = sizeof(struct buddy_t) + __alignof__(struct buddy_t);
	uint64_t buddy_size = buddy_struct_size + BUDDY_ARENA_SIZE;
	uint64_t n_arenas = (uint64_t)(ram_max_addr - (uintptr_t)&kernelend) / buddy_size;

	// check if we can squeeze one more buddy in at the end
	if (ram_max_addr - (n_arenas * buddy_size) > (BUDDY_ARENA_SIZE / 2))
		n_arenas++;

	uint64_t end_of_buddies = (uint64_t)&kernelend + (n_arenas * buddy_struct_size);

	// first buddy
	pages = (struct buddy_t *)&kernelend;
	pages->size = BUDDY_ARENA_SIZE;
	pages->arena = (unsigned char *)end_of_buddies;
	pages->arena += ARCH_PAGE_SIZE - ((uintptr_t)pages->arena % ARCH_PAGE_SIZE); // page align

	buddy_init(pages);

	terminal_logf("Start of pages arena: 0x%x", pages->arena);

	// fill up buddies across the pages

	struct buddy_t *prev = pages;

	for (uint64_t i = 1; i < n_arenas; i++)
	{
		// TODO(tcfw): support memory holes & NUMA layouts

		struct buddy_t *current = (struct buddy_t *)(prev + 1);
		current->size = BUDDY_ARENA_SIZE;
		current->arena = prev->arena + prev->size;
		buddy_init(current);

		// partial buddy
		if ((uintptr_t)(current->arena + current->size) > ram_max_addr)
			current->size = ram_max_addr - (uint64_t)current->arena;

		// terminal_logf("Buddy: *0x%x size: 0x%x arena: 0x%x", current, current->size, current->arena);

		prev->next = current;
		prev = current;
	}

	terminal_logf("End of pages arena: 0x%x", (prev->arena + prev->size));
}

unsigned int size_to_order(size_t size)
{
	unsigned int order = 0;
	size_t block_size = BUDDY_BLOCK_SIZE;
	while (block_size < size)
	{
		block_size *= 2;
		order++;
	}
	return order;
}

struct page *page_alloc_s(size_t size)
{
	unsigned int order = size_to_order(size);
	return (struct page *)page_alloc(order);
}

struct page *page_alloc(unsigned int order)
{
	spinlock_acquire(&page_lock);

	void *addr = buddy_alloc(pages, order);

	spinlock_release(&page_lock);

	return (struct page *)addr;
}

void page_free(void *ptr)
{
	spinlock_acquire(&page_lock);

	buddy_free(pages, ptr);

	spinlock_release(&page_lock);
}