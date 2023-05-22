#ifndef _ARCH_VM_H
#define _ARCH_VM_H

#include "stdint.h"
#include <kernel/list.h>

#define ARCH_PAGE_SIZE 4096
#define RAM_MAX (0x40000000 + 0x20000000)

typedef struct tcr_t
{
	uint64_t _res0 : 4;
	uint64_t ds : 1;
	uint64_t tcma1 : 1;
	uint64_t tcma0 : 1;
	uint64_t e0pd1 : 1;
	uint64_t e0pd0 : 1;
	uint64_t nfd1 : 1;
	uint64_t nfd0 : 1;
	uint64_t tbid1 : 1;
	uint64_t tbid0 : 1;
	uint64_t _res1 : 8;
	uint64_t hpd1 : 1;
	uint64_t hpd0 : 1;
	uint64_t hd : 1;
	uint64_t ha : 1;
	uint64_t tbi1 : 1;
	uint64_t tbi0 : 1;
	uint64_t as : 1;
	uint64_t _res2 : 1;
	uint64_t ips : 4;
	uint64_t tg1 : 2;
	uint64_t sh1 : 2;
	uint64_t orgn1 : 2;
	uint64_t irgn1 : 2;
	uint64_t epd1 : 1;
	uint64_t a1 : 1;
	uint64_t t1sz : 6;
	uint64_t tg0 : 2;
	uint64_t sh0 : 2;
	uint64_t orgn0 : 2;
	uint64_t irgn0 : 2;
	uint64_t epd0 : 1;
	uint64_t _res3 : 1;
	uint64_t t0sz : 6;
} tcr_reg;

// Represents Levels 1, 2 & 3
// Assumed a 52-bit OA
struct __attribute__((packed)) vm_entry_t
{
	uint64_t valid : 1;	  // 1 = valid, 0 = invalid
	uint64_t is_page : 1; // 1 = page, 0 = block

	uint64_t attr : 3;		 // memory attributes
	uint64_t non_secure : 1; // non-secure
	uint64_t ap : 2;		 // access permissions
	uint64_t oa1 : 2;		 // upper bits of oa
	uint64_t af : 1;		 // access flag
	uint64_t ng : 1;		 // non-global

	uint64_t oa : 38;
	// union
	// {
	// 	uint64_t next_level : 38;
	// 	struct
	// 	{
	// 		uint64_t _reserved0 : 4;
	// 		uint64_t nt : 1;
	// 		uint64_t : 0;
	// 		union
	// 		{
	// 			uint64_t l3_block_oa : 29;
	// 			uint64_t l2_block_oa : 20;
	// 			uint64_t l1_block_oa : 11;
	// 		} oa;
	// 	} vm;
	// } oa;

	uint64_t gp : 1;			// guarded page
	uint64_t dbm : 1;			// dirty bit modifier
	uint64_t contiguous : 1;	// contiguous
	uint64_t privilege : 1;		// privilege \/
	uint64_t execute_never : 1; // execute never

	uint64_t allocd : 1;	// if was alloc'd at any point
	uint64_t swapedout : 1; // if has been swapped out
	uint64_t mapped : 1;	// if the region has been mapped, not alloc'd
	uint64_t _ignored1 : 1; // software usage - not used

	uint64_t hw_attrs : 4;	 // reserved
	uint64_t _reserved1 : 1; // reserved
};

struct __attribute__((packed)) vm_table_desc_t
{
	uint64_t valid : 1;			// 1=valid, 0=not valid
	uint64_t is_descriptor : 1; // 1=is descriptor
	uint64_t _ignored0 : 6;		// software usage - not used
	uint64_t next_level1 : 2;	// bits 51:50 of next level
	uint64_t _ignored1 : 2;		// software usage - not used
	uint64_t next_level0 : 38;	// bits 49:12 of next level
	uint64_t _reserved0 : 1;	// reserved
	uint64_t allocd : 1;		// OS meta - has been alloc'd at some point
	uint64_t swapedout : 1;		// OS meta - has been swapped out
	uint64_t _ignored2 : 2;
	uint64_t privilege : 1;		// privilege \/
	uint64_t execute_never : 1; // execute never
	uint64_t permissions : 2;	// access perms
	uint64_t nonsecure : 1;		// none-secure
};

struct vm_table_block_t
{
	struct vm_entry_t entries[512];
};

typedef struct vm_table_t
{
	struct vm_table_desc_t descriptors[512];
} vm_table;

typedef enum
{
	INVALID,
	SHARED,
	NOCACHE,
} entry_state_e;

#endif