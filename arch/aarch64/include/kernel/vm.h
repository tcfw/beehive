#ifndef _ARCH_VM_H
#define _ARCH_VM_H

#include "stdint.h"

#define ARCH_PAGE_SIZE 4096
#define RAM_MAX (0x40000000 + 0x20000000)

// Represents Levels 1, 2 & 3
// Assumed a 52-bit OA
struct vm_entry_t
{
	uint64_t valid : 1;
	uint64_t is_page : 1; // 1 = page, 0 = block

	uint64_t attr : 3;
	uint64_t non_secure : 1;
	uint64_t ap : 2;
	uint64_t oa1 : 2;
	uint64_t af : 1;
	uint64_t ng : 1;

	union
	{
		struct
		{
			uint64_t page_oa : 38;
		};
		struct
		{
			uint64_t _reserved0 : 4;
			uint64_t nt : 1;
			union
			{
				struct
				{
					uint64_t : 0;
					uint64_t block_oa_l3 : 29;
				};
				struct
				{
					uint64_t : 0;
					uint64_t block_oa_l2 : 20;
				};
				struct
				{
					uint64_t : 0;
					uint64_t block_oa_l1 : 11;
				};
			};
		};
	};

	uint64_t gp : 1;
	uint64_t dbm : 1;
	uint64_t contiguous : 1;
	uint64_t privilege : 1;
	uint64_t execute_never : 1;
	uint64_t _ignored1 : 4;
	uint64_t hw_attrs : 4;
	uint64_t _reserved1 : 1;
};

struct vm_table_desc_t
{
	uint64_t valid : 1;
	uint64_t is_descriptor : 1;
	uint64_t _ignored0 : 6;
	uint64_t next_level1 : 2; // bits 51:50 of next level
	uint64_t _ignored1 : 2;
	uint64_t next_level0 : 38; // bits 49:12 of next level
	uint64_t _reserved0 : 1;
	uint64_t _ignored2 : 8;
	uint64_t privilege : 1;
	uint64_t execute_never : 1;
	uint64_t permissions : 2;
	uint64_t nonsecure : 1;
};

struct vm_entry_meta_t
{
	uint16_t inswap : 1;
};

struct vm_table_t
{
	struct vm_table_desc_t descriptors[512];
	struct vm_entry_t blocks_pages[];
};

typedef enum
{
	INVALID,
	SHARED,
	NOCACHE,
} entry_state_e;

uintptr_t vm_translate_to_virtual(struct vm_table_t *table, uintptr_t pptr);
uintptr_t vm_translate_to_physcial(struct vm_table_t *table, uintptr_t vptr);
void vm_map_region(struct vm_table_t *table, uintptr_t pstart, uintptr_t pend, uintptr_t vstart, uintptr_t vend);
void vm_unmap_region(struct vm_table_t *table, uintptr_t vstart, uintptr_t vend);
void vm_mark_region(struct vm_table_t *table, entry_state_e state, uintptr_t page);

void vm_set_table(struct vm_table_t *table);
void vm_clear_caches();

#endif