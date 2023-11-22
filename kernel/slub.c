#include <kernel/slub.h>
#include <kernel/list.h>
#include <kernel/mm.h>
#include <kernel/arch.h>
#include <kernel/paging.h>
#include "devicetree.h"

void slub_init(slub_t *slub)
{
	slub->object_count = 0;
	slub->cache_alloc_order = (1 << (slub->object_size / PAGE_SIZE)) >> 1;

	INIT_LIST_HEAD(&slub->partial);
	slub->per_cpu_partial = (slub_cache_entry_t **)((char *)slub + sizeof(slub_t));
}

static void slub_init_cache_entry(slub_t *slub, slub_cache_entry_t *cache_entry)
{
	memset(cache_entry, 0, PAGE_SIZE);
	cache_entry->slub = slub;

	int reserved_area = sizeof(slub_cache_entry_t);
	cache_entry->first = (void *)(reserved_area + (char *)cache_entry);

	unsigned int obj_count = (((1 << slub->cache_alloc_order) * PAGE_SIZE) - reserved_area) / slub->object_size;
	cache_entry->used = 0;

	slub_entry_t *prev = 0;
	for (int i = 0; i < obj_count; i++)
	{
		slub_entry_t *entry = (slub_entry_t *)(((char *)cache_entry->first + (i * slub->object_size)));

		if (slub->object_size >= sizeof(slub_entry_t))
			entry->poison = POISON_VALUE;

		if (prev != 0)
			prev->next_offset = entry;

		prev = entry;
	}

	prev->next_offset = 0;
	if (slub->object_size >= sizeof(slub_entry_t))
		prev->poison = POISON_VALUE;
}

static void *slub_alloc_from_cache_entry(slub_t *slub, slub_cache_entry_t *entry)
{
	if (entry->first == 0)
		return 0;

	int state = spinlock_acquire_irq(&entry->lock);

	slub_entry_t *addr = (slub_entry_t *)entry->first;
	entry->first = addr->next_offset;
	entry->used++;

	spinlock_release_irq(state, &entry->lock);

	return addr;
}

static slub_cache_entry_t *slub_get_per_cpu_cache(slub_t *slub)
{
	return slub->per_cpu_partial[cpu_id()];
}

static void slub_set_per_cpu_cache(slub_t *slub, slub_cache_entry_t *cache)
{
	slub->per_cpu_partial[cpu_id()] = cache;
}

static slub_cache_entry_t *slub_new_cache_entry(slub_t *slub)
{
	struct page *cache = page_alloc(slub->cache_alloc_order);
	slub_init_cache_entry(slub, &cache->slub);

	return &cache->slub;
}

static int slub_cache_entry_is_empty(slub_t *slub, slub_cache_entry_t *entry)
{
	return entry->used == 0;
}

void *slub_alloc(slub_t *slub)
{
	void *addr;
	slub_cache_entry_t *cpu_cache = slub_get_per_cpu_cache(slub);

	if (cpu_cache != 0)
	{
		// alloc from cpu cache
		addr = slub_alloc_from_cache_entry(slub, cpu_cache);
	}
	else if (!list_is_empty(&slub->partial))
	{
		// alloc from partial cache
		int sstate = spinlock_acquire_irq(&slub->lock);

		slub_cache_entry_t *cache = (slub_cache_entry_t *)slub->partial.next;
		addr = slub_alloc_from_cache_entry(slub, cache);
		if (cache->first == 0)
			list_del(cache);

		spinlock_release_irq(sstate, &slub->lock);
	}
	else
	{
		// alloc new cache
		cpu_cache = slub_new_cache_entry(slub);
		slub_set_per_cpu_cache(slub, cpu_cache);
		addr = slub_alloc_from_cache_entry(slub, cpu_cache);
	}

	memset(addr, 0, slub->object_size);

	int state = spinlock_acquire_irq(&slub->lock);
	slub->object_count++;
	spinlock_release_irq(state, &slub->lock);

	// if cache entry full
	if (cpu_cache != 0 && cpu_cache->first == 0)
		slub_set_per_cpu_cache(slub, 0);

	if (slub->ctor != 0)
		slub->ctor(addr);

	return addr;
}

static int slub_is_per_cpu_cache(slub_t *slub, slub_cache_entry_t *entry)
{
	int cpuN = devicetree_count_dev_type("cpu");

	for (int i = 0; i < cpuN; i++)
	{
		if (slub->per_cpu_partial[i] == entry)
			return 1;
	}

	return 0;
}

void slub_free(void *obj)
{
	int lstate_c, lstate_n;

	slub_cache_entry_t *cache = (void *)obj - ((uintptr_t)obj % PAGE_SIZE);
	slub_t *slub = cache->slub;
	slub_entry_t *entry = (slub_entry_t *)obj;

	lstate_c = spinlock_acquire_irq(&cache->lock);

	if (cache->first == 0)
	{
		// cache was full
		// so shouldn't be a per-cpu cache anymore
		lstate_n = spinlock_acquire_irq(&slub->lock);
		list_add(cache, &slub->partial);
		spinlock_release_irq(lstate_n, &slub->lock);
	}

	entry->next_offset = cache->first;
	cache->first = entry;

	if (slub->object_size >= sizeof(slub_entry_t))
		entry->poison = POISON_VALUE;

	cache->used--;

	spinlock_release_irq(lstate_c, &cache->lock);

	lstate_n = spinlock_acquire_irq(&slub->lock);

	if ((slub_cache_entry_is_empty(slub, cache) == 0) && (slub_is_per_cpu_cache(slub, cache) == 0))
	{
		list_del((struct list_head *)cache);
		page_free((void *)cache);
	}

	slub->object_count--;

	spinlock_release_irq(lstate_n, &slub->lock);
}

slub_t *DEFINE_DYN_SLUB(unsigned int objsize)
{
	slub_t *slub = page_alloc_s(sizeof(slub_t));
	memset(slub, 0, PAGE_SIZE);

	slub->object_size = objsize;
	slub->cache_alloc_order = (1 << (objsize / PAGE_SIZE)) >> 1;
	slub_init(slub);

	return slub;
}