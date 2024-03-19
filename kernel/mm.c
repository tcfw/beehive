#include <kernel/arch.h>
#include <kernel/buddy.h>
#include <kernel/devicetree.h>
#include <kernel/mm.h>
#include <kernel/regions.h>
#include <kernel/slub.h>
#include <kernel/stdint.h>
#include <kernel/strings.h>
#include <kernel/sync.h>
#include <kernel/tty.h>
#include <kernel/vm.h>

extern uint64_t kernelend;
extern uint64_t kernelvstart;

struct buddy_t *pages;
static spinlock_t page_lock;

slub_t *slub_head;

#define MAX_SLUB_CLASSES (12)

const slub_class_catalogue_t slub_classes[MAX_SLUB_CLASSES] = {
	{8},
	{16},
	{24},
	{32},
	{40},
	{56},
	{72},
	{88},
	{128},
	{1024},
	{2048},
	{3072},
	{PAGE_SIZE - sizeof(slub_cache_entry_t)},
};

slub_t *get_slub_head()
{
	return slub_head;
}

struct buddy_t *get_pages_head()
{
	return pages;
}

void *page_start_of_arena()
{
	return (void *)pages->arena;
}

void page_alloc_init()
{
	spinlock_init(&page_lock);

	uintptr_t ram_max_addr = VIRT_OFFSET + ram_max();
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
	pages->arena += PAGE_SIZE - ((uintptr_t)pages->arena % PAGE_SIZE); // page align

	buddy_init(pages);

	terminal_logf("Start of pages arena: 0x%x", (uintptr_t)pages->arena);

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

	terminal_logf("End of pages arena: 0x%x", (uintptr_t)(prev->arena + prev->size));
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

struct page *__attribute__((malloc)) page_alloc_s(size_t size)
{
	unsigned int order = size_to_order(size);
	return (struct page *)page_alloc(order);
}

struct page *__attribute__((malloc)) page_alloc(unsigned int order)
{
	int state = spinlock_acquire_irq(&page_lock);

	void *addr = buddy_alloc(pages, order);

	spinlock_release_irq(state, &page_lock);

	return (struct page *)addr;
}

void page_reloc(uintptr_t offset)
{
	pages = (struct buddy_t *)((void *)pages + offset);

	struct buddy_t *current = pages;

	while (current != 0)
	{
		current->arena = (unsigned char *)((void *)current->arena + offset);
		if (current->next != 0)
			current->next = (struct buddy_t *)((void *)current->next + offset);

		current = current->next;
	}
}

void page_free(void *ptr)
{
	int state = spinlock_acquire_irq(&page_lock);

	buddy_free(pages, ptr);

	spinlock_release_irq(state, &page_lock);
}

void slub_alloc_init(void)
{
	int cpuN = devicetree_count_dev_type("cpu");

	int slub_size = sizeof(slub_t) + (sizeof(slub_cache_entry_t *) * cpuN);
	slub_size += CACHE_LINE_SIZE - (slub_size % CACHE_LINE_SIZE);

	char *slubs = (char *)page_alloc_s(slub_size * MAX_SLUB_CLASSES);
	slub_t *prev = 0;

	for (int i = 0; i < MAX_SLUB_CLASSES; i++)
	{
		slub_t *slub = (slub_t *)(slubs + (slub_size * i));
		slub->object_size = slub_classes[i].object_size;

		slub_init(slub);

		if (prev != 0)
			prev->next = slub;
		else
			slub_head = slub;

		prev = slub;
	}
}

void *__attribute__((malloc)) kmalloc(size_t size)
{
	slub_t *slub_class = slub_head;

	while (slub_class)
	{
		if (size <= (size_t)slub_class->object_size)
			return slub_alloc(slub_class);

		slub_class = slub_class->next;
	}

	return 0;
}

void kfree(void *obj)
{
	return slub_free(obj);
}