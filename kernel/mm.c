#include <kernel/buddy.h>
#include <kernel/mm.h>
#include <kernel/vm.h>
#include <kernel/tty.h>
#include <kernel/strings.h>
#include <kernel/sync.h>
#include "stdint.h"

extern uint64_t kernelend;

struct buddy_t *pages;
static spinlock_t page_lock;

void page_alloc_init()
{
	spinlock_init(&page_lock);

	// TODO(tcfw): use DTB to find allocatable areas
	terminal_logf("Start of pages: 0x%x", &kernelend);

	// first buddy
	pages = (struct buddy_t *)&kernelend;
	pages->arena = (unsigned char *)(pages + sizeof(struct buddy_t) + __alignof__(struct buddy_t));
	pages->size = BUDDY_ARENA_SIZE;
	buddy_init(pages);

	struct buddy_t *prev = pages;

	// TODO(tcfw): move buddy metadata next to each other so arenas are contiguous

	// fill up buddies across the pages
	// TODO(tcfw): support memory holes
	while ((uintptr_t)(prev->arena + prev->size) < RAM_MAX)
	{
		struct buddy_t *current = (struct buddy_t *)(prev->arena + prev->size);
		current->arena = (unsigned char *)(current + sizeof(struct buddy_t) + __alignof__(struct buddy_t));
		current->size = BUDDY_ARENA_SIZE;

		if ((uintptr_t)(current->arena + current->size) >= RAM_MAX)
		{
			// partial buddy
			current->size = RAM_MAX - (uint64_t)current->arena;

			terminal_logf("Partial buddy at 0x%x size: 0x%x", prev->arena, prev->size);
		}

		buddy_init(current);
		prev->next = current;
		prev = current;
	}

	terminal_logf("End of pages: 0x%x", (prev->arena + prev->size));
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

void *page_alloc_s(size_t size)
{
	unsigned int order = size_to_order(size);
	return page_alloc(order);
}

void *page_alloc(unsigned int order)
{
	spinlock_acquire(&page_lock);

	void *addr = buddy_alloc(pages, order);

	spinlock_release(&page_lock);

	return addr;
}

void page_free(void *ptr)
{
	spinlock_acquire(&page_lock);

	buddy_free(pages, ptr);

	spinlock_release(&page_lock);
}