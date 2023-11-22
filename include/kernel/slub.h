#ifndef _KERNEL_SLUB_H
#define _KERNEL_SLUB_H

#include <kernel/list.h>
#include <kernel/sync.h>
#include <kernel/paging.h>
#include "stdint.h"

/*

SLUB memory allocator

Entry
- The smallest entry can be 8 bytes
- If empty, struct slub_entry_t will be held in the objects data location
- Each next_offset is the number of entries to the next free entry, not number
  of bytes
Empty entries:
-------------------------------------------------
| next_offset | poison |...       | next_offset |
-------------------------------------------------
	(1) ---------------------------^ (4) ------->

Entry poisoning
If the entries are larger than 8 bytes, a
poisoning value can be inserted after next_offset.
This can be useful for catching overflows.

Cache Entry
- Prev & next point to the next cache entries
- first points to the first empty entry
- first will be 0 (null) is cache entry is full
- Will be allocated onto at least one whole page by
  the buddy allocator
- Depending on object size, may take up more than 1 page
-------------------------------------------------
| *prev | *next | *first | object | objects ... |
-------------------------------------------------
					|--------^

Slubs:
- Hold the partial cache entries
- Empty caches are returned back to the buddy allocator
- Full cache entries are not tracked, on free they will be added
  back into the partial cache entries
- Hold metadata about the objects being allocated
- Only hold one object size each (e.g. 8, 16, 32... 1024 etc bytes)
- Can only hold a max PAGE_SIZE object
- Points to the next slub for the next size
- cache_alloc_order is the order of pages to use to create a
  new empty/partial cache entry
- object_size is the size of the objects in bytes
- object_count is the number of active objects in the slub
*/

#define POISON_VALUE (0x6C6C6C6CUL)

typedef struct slub_entry_t
{
	void *next_offset;
	uint32_t poison;
} slub_entry_t;

typedef struct slub_t slub_t;

typedef struct slub_cache_entry_t
{
	struct list_head list;
	spinlock_t lock;

	slub_t *slub;

	uint32_t used;

	void *first;

	char padding[16];
} slub_cache_entry_t;

typedef struct slub_t slub_t;

typedef struct slub_t
{
	slub_t *next;

	void (*ctor)(void *);

	unsigned int cache_alloc_order;
	unsigned int object_size;
	unsigned int object_count;

	spinlock_t lock;
	struct list_head partial;

	slub_cache_entry_t **per_cpu_partial;
} slub_t;

typedef struct slub_class_catalogue_t
{
	unsigned int object_size
} slub_class_catalogue_t;

slub_t *DEFINE_DYN_SLUB(unsigned int objsize);

void slub_init(slub_t *slub);

void *slub_alloc(slub_t *slub);

void slub_free(void *);

#endif