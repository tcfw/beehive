#include <kernel/vm.h>
#include <kernel/mm.h>
#include <kernel/tty.h>

const uint64_t kernelstart = 0x40000000;
extern uint64_t kernelend;

vm_table *kernel_vm_map;

static void vm_free_table_block(struct vm_table_block_t *block);
static struct vm_table_block_t *vm_table_desc_to_block(struct vm_table_desc_t *desc);
static struct vm_table_block_t *vm_table_entry_to_block(struct vm_entry_t *entry);
static struct vm_table_block_t *vm_get_or_alloc_block(struct vm_table_block_t *parent, uint16_t entry);

static struct vm_table_block_t *vm_table_desc_to_block(struct vm_table_desc_t *desc)
{
	return (struct vm_table_block_t *)(desc->next_level1 << 50 | desc->next_level0 << 12);
}

static struct vm_table_block_t *vm_table_entry_to_block(struct vm_entry_t *entry)
{
	return (struct vm_table_block_t *)(entry->oa1 << 50 | entry->oa << 12);
}

void vm_init()
{
	kernel_vm_map = (vm_table *)page_alloc_s(sizeof(vm_table));

	// map kernel code regions
	vm_map_region(kernel_vm_map, kernelstart, kernelend, kernelstart, kernelend);

	// map remaining physical memory
	vm_map_region(kernel_vm_map, kernelend, RAM_MAX, kernelend, RAM_MAX);

	// Setup TCR_EL1
	volatile tcr_reg tcr = {0};
	__asm__ volatile("mrs %0, TCR_EL1" ::"r"(tcr));

	// TODO(tcfw) caching controls!

	tcr.ds = 1;		 // enable 52-bit OA
	tcr.e0pd1 = 1;	 // generate fault if TTBR1 is accessed in EL0
	tcr.e0pd0 = 0;	 // dont generate fault if TTBR0 is accessed in EL0
	tcr.as = 1;		 // AS - ASID is 16 bits
	tcr.ips = 0b110; // Intermediate physcial address size - 52 bits
	tcr.tg1 = 0b10;	 // TTBR1 Page granule size - 4KB
	tcr.a1 = 0;		 // ASID defined in TTBR0
					 // TODO(tcfw): tcr.t1sz
	tcr.tg0 = 0b10;	 // TTBR0 Page granule size - 4KB
					 // TODO(tcfw): tcr.t0sz

	__asm__ volatile("msr TCR_EL1, %0"
					 : "=r"(tcr));
}

void vm_init_table(vm_table *table)
{
}

uintptr_t vm_va_to_pa(vm_table *table, uintptr_t vptr)
{
	uint16_t l0 = (vptr & 0xFF8000000000) >> 39;
	uint16_t l1 = (vptr & 0x007FC0000000) >> 30;
	uint16_t l2 = (vptr & 0x00003FE00000) >> 21;
	uint16_t l3 = (vptr & 0x0000001FF000) >> 12;
	uint16_t pageoff = (vptr & 0xFFF);

	if (l0 > 511 | l1 > 511 | l2 > 511 | l3 > 511)
	{
		return 0;
	}

	if (table->descriptors[l0].allocd == 1)
	{
		// l1
		struct vm_table_block_t *block = vm_table_desc_to_block(&table->descriptors[l0]);
		if (block->entries[l1].allocd == 0)
		{
			return 0;
		}

		if (block->entries[l1].is_page == 1)
		{
			return (block->entries[l1].oa1 << 50 | (block->entries[l1].oa & 0x7FE0000000) | (vptr & 0x3FFFFFFFFF));
		}

		// l2
		block = vm_table_entry_to_block(&block->entries[l1]);
		if (block->entries[l2].allocd == 0)
		{
			return 0;
		}

		if (block->entries[l2].is_page == 1)
		{
			return (block->entries[l2].oa1 << 50 | (block->entries[l2].oa & 0x7FFFF00000) | (vptr & 0xFFFFF));
		}

		// l3
		block = vm_table_entry_to_block(&block->entries[l2]);
		if (block->entries[l3].allocd == 0)
		{
			return 0;
		}

		if (block->entries[l3].is_page == 1)
		{
			return (block->entries[l3].oa1 << 50 | (block->entries[l3].oa & 0x7FFFFFF000) | pageoff);
		}
	}

	return 0;
}

static void vm_free_table_block(struct vm_table_block_t *block)
{
	for (uint8_t i = 0; i < 512; i++)
	{
		if (block->entries[i].allocd == 1 && block->entries[i].is_page == 0)
		{
			vm_free_table_block(vm_table_entry_to_block(&block->entries[i]));
		}
	}

	page_free(block);
}

void vm_free_table(vm_table *table)
{
	for (uint8_t i = 0; i < 512; i++)
	{
		if (table->descriptors[i].allocd == 1)
		{
			vm_free_table_block(vm_table_desc_to_block(&table->descriptors[i]));
		}
	}

	page_free(table);
}

static struct vm_table_block_t *vm_get_or_alloc_block(struct vm_table_block_t *parent, uint16_t entry)
{
	if (entry > 511)
	{
		return 0;
	}

	struct vm_table_block_t *block;

	if (parent->entries[entry].allocd == 0)
	{
		block = (struct vm_table_block_t *)page_alloc_s(sizeof(struct vm_table_block_t));
		parent->entries[entry].allocd = 1;
		parent->entries[entry].valid = 1;
		parent->entries[entry].non_secure = 1;
		parent->entries[entry].oa = ((uintptr_t)block >> 12) & 0x3FFFFFFFFF;
		parent->entries[entry].oa1 = ((uintptr_t)block >> 50) & 0x2;
	}
	else
	{
		block = vm_table_entry_to_block(&parent->entries[entry]);
	}

	return block;
}

int vm_map_region(vm_table *table, uintptr_t pstart, uintptr_t pend, uintptr_t vstart, uintptr_t vend)
{
	// TODO(tcfw) check vstart and vend is page aligned
	if (vstart & 0xFFF != 0 || vend & 0xfff != 0)
	{
		terminal_log("WARN: vm region map was not page aligned");
		return -1;
	}

	if ((vend - vstart) != (pend - pstart))
	{
		terminal_log("WARN: vm region map virtual size does not match physcial size");
		return -1;
	}

	pstart >>= 12;

	uint16_t l0 = (vstart & 0xFF8000000000) >> 39;
	uint16_t l1 = (vstart & 0x007FC0000000) >> 30;
	uint16_t l2 = (vstart & 0x00003FE00000) >> 21;
	uint16_t l3 = (vstart & 0x0000001FF000) >> 12;

	struct vm_entry_t *vpage;
	struct vm_table_block_t *table_l1;
	struct vm_table_block_t *table_l2;
	struct vm_table_block_t *table_l3;

	if (table->descriptors[l0].allocd == 0)
	{
		// alloc block
		table_l1 = (struct vm_table_block_t *)page_alloc_s(sizeof(struct vm_table_block_t));
		table->descriptors[l0].valid = 1;
		table->descriptors[l0].allocd = 1;
		table->descriptors[l0].is_descriptor = 1;
		table->descriptors[l0].nonsecure = 1;
		table->descriptors[l0].next_level0 = ((uintptr_t)table_l1 >> 12) & 0x3FFFFFFFFF;
		table->descriptors[l0].next_level1 = ((uintptr_t)table_l1 >> 50) & 0x2;
	}
	else
	{
		table_l1 = vm_table_desc_to_block(&table->descriptors[l0]);
	}

	table_l2 = vm_get_or_alloc_block(table_l1, l1);
	table_l3 = vm_get_or_alloc_block(table_l2, l2);

	while (vstart < vend)
	{
		vpage = &table_l3->entries[l3];
		vpage->is_page = 1;
		vpage->valid = 1;
		vpage->non_secure = 1;
		vpage->oa1 = (pstart & 0x3000000000000) >> 48;
		vpage->oa = pstart & 0x1FFFFFFFFF;

		// TODO(tcfw) memory flags
		// TODO(tcfw) access perms
		// TODO(tcfw) check if contiguous

		l3++;

		if (l3 > 511)
		{
			l3 = 0;
			l2++;
			table_l3 = vm_get_or_alloc_block(table_l2, l2);
		}

		if (l2 > 511)
		{
			l2 = 0;
			l1++;
			table_l2 = vm_get_or_alloc_block(table_l1, l1);
			table_l3 = vm_get_or_alloc_block(table_l2, l2);
		}

		if (l1 > 512)
		{
			l1 = 0;
			l0++;
			table_l1 = vm_table_desc_to_block(&table->descriptors[l0]);
			table_l2 = vm_get_or_alloc_block(table_l1, l1);
			table_l3 = vm_get_or_alloc_block(table_l2, l2);
		}

		vstart += ARCH_PAGE_SIZE;
		pstart++;
	}
}