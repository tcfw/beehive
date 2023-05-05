#include <kernel/buddy.h>
#include <kernel/mm.h>
#include <kernel/vm.h>
#include <kernel/tty.h>
#include <kernel/strings.h>
#include <kernel/sync.h>
#include "stdint.h"

extern uint64_t kernelend;

static spinlock_t page_lock;

struct buddy_t *pages;

void page_alloc_init()
{
	spinlock_init(&page_lock);

	// TODO(tcfw): use DTB to find allocatable areas
	char buf[50];
	ksprintf(&buf[0], "Start of pages: 0x%x", &kernelend);
	terminal_log(buf);

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

			ksprintf(&buf[0], "Partial buddy at 0x%x size: 0x%x", prev->arena, prev->size);
			terminal_log(buf);
		}

		buddy_init(current);
		prev->next = current;
		prev = current;
	}

	ksprintf(&buf[0], "End of pages: 0x%x", (prev->arena + prev->size));
	terminal_log(buf);
}

void *alloc_page(unsigned int order)
{
	spinlock_acquire(&page_lock);

	void *addr = buddy_alloc(pages, order);

	spinlock_release(&page_lock);

	return addr;
}