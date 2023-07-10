#include <kernel/vm.h>
#include <kernel/mm.h>
#include <kernel/tty.h>
#include <kernel/regions.h>

const uint64_t kernelstart = 0x40000000;
extern uint64_t kernelend;

vm_table *kernel_vm_map;

static void vm_free_table_block(volatile vm_table_block *block);
static vm_table_block *vm_table_desc_to_block(uint64_t *desc);
static vm_table_block *vm_table_entry_to_block(uint64_t *entry);
static vm_table_block *vm_get_or_alloc_block(volatile vm_table_block *parent, uint16_t entry, uint8_t level);

static vm_table_block *vm_table_desc_to_block(uint64_t *desc)
{
	return (vm_table_block *)(*desc & VM_ENTRY_OA_MASK);
}

static vm_table_block *vm_table_entry_to_block(uint64_t *entry)
{
	return (vm_table_block *)(*entry & VM_ENTRY_OA_MASK);
}

vm_table *vm_get_current_table()
{
	return kernel_vm_map;
}

void vm_init()
{
	kernel_vm_map = (vm_table *)page_alloc_s(sizeof(vm_table));

	// map device space
	if (vm_map_region(kernel_vm_map, 0x9000000, DEVICE_REGION, 4095, MEMORY_TYPE_KERNEL | MEMORY_TYPE_DEVICE | MEMORY_PERM_RW) < 0)
		terminal_log("failed to map device region");

	// map kernel code & remaining physcial memory regions
	if (vm_map_region(kernel_vm_map, kernelstart, kernelstart, RAM_MAX - kernelstart - 1, MEMORY_TYPE_KERNEL | MEMORY_PERM_RW) < 0)
		terminal_log("failed to map kernel code region");

	// move DBT to above RAM
	if (vm_map_region(kernel_vm_map, 0, RAM_MAX, 0x100000 - 1, MEMORY_TYPE_KERNEL | MEMORY_TYPE_DEVICE | MEMORY_PERM_RO) < 0)
		terminal_log("failed to map dbt region");

	__asm__ volatile("ISB");
	terminal_log("Loaded kernel VM map");
}

void vm_init_table(vm_table *table)
{
}

void vm_set_kernel()
{
	vm_set_table(kernel_vm_map);
}

uintptr_t vm_va_to_pa(vm_table *table, uintptr_t vptr)
{
	uint16_t l0 = vptr >> 39;
	uint16_t l1 = (vptr >> 30) & 0x1FF;
	uint16_t l2 = (vptr >> 21) & 0x1FF;
	uint16_t l3 = (vptr >> 12) & 0x1FF;
	uint16_t pageoff = (vptr & 0xFFF);

	if ((l0 > 511) | (l1 > 511) | (l2 > 511) | (l3 > 511))
	{
		return 0;
	}

	if ((table->descriptors[l0] & VM_DESC_ALLOCD) != 0)
	{
		vm_table_block *block = vm_table_desc_to_block(&table->descriptors[l0]);

		// l1
		if ((block->entries[l1] & VM_ENTRY_ALLOCD) == 0)
			return 0;

		if ((block->entries[l1] & VM_ENTRY_ISTABLE) == 0)
			return ((uintptr_t)(block->entries[l1] & 0x7FE0000000) | (vptr & 0x3FFFFFFFFF));

		// l2
		block = vm_table_entry_to_block(&block->entries[l1]);
		if ((block->entries[l2] & VM_ENTRY_ALLOCD) == 0)
			return 0;

		if ((block->entries[l2] & VM_ENTRY_ISTABLE) == 0)
			return ((uintptr_t)(block->entries[l2] & 0x7FFFF00000) | (vptr & 0xFFFFF));

		// l3
		block = vm_table_entry_to_block(&block->entries[l2]);
		if ((block->entries[l3] & VM_ENTRY_ALLOCD) == 0)
			return 0;

		if ((block->entries[l3] & VM_ENTRY_ISTABLE) == 0)
			return ((uintptr_t)(block->entries[l3] & 0x7FFFFFF000) | pageoff);
	}

	return 0;
}

static void vm_free_table_block(volatile vm_table_block *block)
{
	// for (uint8_t i = 0; i < 512; i++)
	// {
	// 	if (block->entries[i].allocd == 1 && block->entries[i].is_page == 0)
	// 	{
	// 		vm_free_table_block(vm_table_entry_to_block(&block->entries[i]));
	// 	}
	// }

	// page_free(block);
}

void vm_free_table(vm_table *table)
{
	// for (uint8_t i = 0; i < 512; i++)
	// {
	// 	if (table->descriptors[i].allocd == 1)
	// 	{
	// 		vm_free_table_block(vm_table_desc_to_block(&table->descriptors[i]));
	// 	}
	// }

	// page_free(table);
}

static vm_table_block *vm_get_or_alloc_block(volatile vm_table_block *parent, uint16_t entry, uint8_t level)
{
	if (entry > 511)
	{
		return 0;
	}

	vm_table_block *block;

	if ((parent->entries[entry] & VM_ENTRY_ALLOCD) == 0)
	{
		block = (vm_table_block *)page_alloc_s(sizeof(vm_table_block));

		parent->entries[entry] = (uintptr_t)block & VM_ENTRY_OA_MASK;
		parent->entries[entry] |= (VM_ENTRY_ALLOCD | VM_ENTRY_ISTABLE | VM_ENTRY_VALID | VM_ENTRY_NONSECURE);
	}
	else
	{
		block = vm_table_entry_to_block(&parent->entries[entry]);
	}

	return block;
}

int vm_map_region(vm_table *table, uintptr_t pstart, uintptr_t vstart, size_t size, uint64_t flags)
{
	uint64_t vend = vstart + size;

	// check vstart and vend is page aligned
	if ((vstart & 0xFFF) != 0 || (vend & 0xFFF) != 0xFFF)
	{
		terminal_log("WARN: vm region map was not page aligned");
		return -1;
	}

	uint16_t l0 = vstart >> 39;
	uint16_t l1 = (vstart >> 30) & 0x1FF;
	uint16_t l2 = (vstart >> 21) & 0x1FF;
	uint16_t l3 = (vstart >> 12) & 0x1FF;

	vm_table_block *table_l1 = 0;
	vm_table_block *table_l2 = 0;
	vm_table_block *table_l3 = 0;
	volatile uint64_t *vpage = 0;
	volatile uint64_t *desc = &table->descriptors[l0];

	if ((*desc & VM_DESC_ALLOCD) == 0)
	{
		// alloc block
		table_l1 = (vm_table_block *)page_alloc_s(sizeof(vm_table_block));
		*desc = (VM_DESC_VALID | VM_DESC_IS_DESC | VM_DESC_ALLOCD | VM_DESC_NONSECURE | VM_DESC_AF | VM_ENTRY_ISH);
		*desc |= ((uintptr_t)table_l1 & VM_DESC_NEXT_LEVEL_MASK);

		if (flags & MEMORY_TYPE_KERNEL)
		{
			// *desc |= VM_DESC_UXN;
			// desc->permissions = 0;		  // EL0 No read

			if (flags & MEMORY_PERM_RO)
			{
				// *desc |= VM_DESC_PXN;

				// desc->privilege_execute_never = 1; // ELx Not executable
				// desc->permissions = 0b10;		   // RO, EL0 No read
			}
		}
		else if (flags & MEMORY_TYPE_USER)
		{
			// desc->permissions = 0b01; // RO, EL1+0 read/write

			// if (flags & MEMORY_PERM_RO)
			// {
			// 	desc->user_execute_never = 1; // ELx Not executable
			// 	desc->permissions = 0b11;	  // RO, EL1+0 read only
			// }
			*desc |= VM_ENTRY_USER;
		}
		if (flags & MEMORY_TYPE_DEVICE)
		{
			*desc |= (1 << VM_DESC_ATTR) | VM_ENTRY_OSH;
		}
	}
	else
	{
		table_l1 = vm_table_desc_to_block(&table->descriptors[l0]);
	}

	while (vstart < vend)
	{
		int level = 3;
		int range = vend - vstart;
		int incsize = ARCH_PAGE_SIZE;
		uint64_t addr = pstart & VM_ENTRY_OA_MASK;

		if (range > L1_BLOCK_SIZE && l3 == 0 && l2 == 0)
		{
			level = 1;
			incsize = L1_BLOCK_SIZE + 1;
			vpage = &table_l1->entries[l1];
		}
		else if (range > L2_BLOCK_SIZE && l3 == 0)
		{
			if (!table_l2)
				table_l2 = vm_get_or_alloc_block(table_l1, l1, 1);

			level = 2;
			incsize = L2_BLOCK_SIZE + 1;
			vpage = &table_l2->entries[l2];
		}
		else // if (range > L1_BLOCK_SIZE && l3 == 0 && l2 == 0)
		{
			if (!table_l2)
				table_l2 = vm_get_or_alloc_block(table_l1, l1, 1);

			if (!table_l3)
				table_l3 = vm_get_or_alloc_block(table_l2, l2, 2);

			vpage = &table_l3->entries[l3];
		}
		// else if (range > L0_BLOCK_SIZE && l3 == 0 && l2 == 0 && l1 == 0)
		// {
		// 	level = 0;
		// 	incsize = L0_BLOCK_SIZE;
		// 	vpage = &table->descriptors[l0];
		// }

		if ((*vpage & VM_ENTRY_MAPPED) > 0)
		{
			terminal_log("WARN: vm region map aleady mapped");
			return -2;
		}
		*vpage = 0;

		*vpage = (VM_ENTRY_VALID | VM_ENTRY_MAPPED | VM_ENTRY_NONSECURE | VM_ENTRY_AF | VM_ENTRY_ISH);
		if (flags & MEMORY_TYPE_DEVICE)
			*vpage |= (1 << VM_ENTRY_ATTR) | VM_ENTRY_OSH;
		else
			*vpage |= (0 << VM_ENTRY_ATTR);

		*vpage |= addr;

		if (level == 3)
		{
			*vpage |= VM_ENTRY_ISTABLE;
		}

		// TODO(tcfw) memory flags
		// TODO(tcfw) access perms
		// TODO(tcfw) check if contiguous

		if (flags & MEMORY_TYPE_USER)
		{
			*vpage |= VM_ENTRY_USER;
		}

		// terminal_logf("mapped memory v:0x%x to p:0x%x at level %x in pte %x (%x %x %x %x): %x", vstart, pstart, level, vpage, l0, l1, l2, l3, *vpage);

		switch (level)
		{
		case 3:
			l3++;
			break;
		case 2:
			l2++;
			break;
		case 1:
			l1++;
			break;
		case 0:
			l0++;
			break;
		}

		if (l3 > 511)
		{
			l3 = 0;
			l2++;
			table_l3 = vm_get_or_alloc_block(table_l2, l2, 2);
		}

		if (l2 > 511)
		{
			l2 = 0;
			l1++;
			table_l2 = vm_get_or_alloc_block(table_l1, l1, 1);
			table_l3 = vm_get_or_alloc_block(table_l2, l2, 2);
		}

		if (l1 > 512)
		{
			l1 = 0;
			l0++;
			table_l1 = vm_table_desc_to_block(&table->descriptors[l0]);
			table_l2 = vm_get_or_alloc_block(table_l1, l1, 1);
			table_l3 = vm_get_or_alloc_block(table_l2, l2, 2);
		}

		vstart += incsize;
		pstart += incsize;
	}

	return 0;
}

void vm_set_table(vm_table *table)
{
	uint64_t ttbr0 = ((uintptr_t)table & TTBR_BADDR_MASK) + 1;
	__asm__ volatile("MSR TTBR0_EL1, %0" ::"r"(ttbr0));

	// invalidate TLBs
	__asm__ volatile("TLBI VMALLE1");
	__asm__ volatile("DSB ISH");
	__asm__ volatile("ISB");
}

void vm_enable()
{
	// Set up MAIR_EL1
	uint64_t mair = (0xFF << 0) | // AttrIdx=0: normal, IWBWA, OWBWA, NTR
					(0x04 << 8) | // AttrIdx=1: device, nGnRE (must be OSH too)
					(0x44 << 16); // AttrIdx=2: non cacheable
	asm volatile("msr mair_el1, %0"
				 :
				 : "r"(mair));

	// Setup TCR_EL1
	uint64_t tcr = (TCR_AS_16BIT_ASID | TCR_IPS_44BITS |
					TCR_TG1_GRANULE_4KB | TCR_A1_ASID1 |
					TCR_TG0_GRANULE_4KB | (20 << TCR_T1SZ_SHIFT) |
					(20 << TCR_T0SZ_SHIFT) | TCR_SH0_INNER | TCR_IRGN0_WRITEBACK |
					TCR_ORGN0_WRITEBACK);

	// uint64_t b = 0;
	// __asm__ volatile("mrs %0, id_aa64mmfr0_el1"
	// 				 : "=r"(b));
	// b &= 0xF;

	// uint64_t tcr = (0b00LL << 37) | // TBI=0, no tagging
	// 			   (b << 32) |		// IPS=autodetected
	// 			   (0b10LL << 30) | // TG1=4k
	// 			   (0b11LL << 28) | // SH1=3 inner
	// 			   (0b01LL << 26) | // ORGN1=1 write back
	// 			   (0b01LL << 24) | // IRGN1=1 write back
	// 			   (0b0LL << 23) |	// EPD1 enable higher half
	// 			   (25LL << 16) |	// T1SZ=25, 3 levels (512G)
	// 			   (0b00LL << 14) | // TG0=4k
	// 			   (0b11LL << 12) | // SH0=3 inner
	// 			   (0b01LL << 10) | // ORGN0=1 write back
	// 			   (0b01LL << 8) |	// IRGN0=1 write back
	// 			   (0b0LL << 7) |	// EPD0 enable lower half
	// 			   (25LL << 0);		// T0SZ=25, 3 levels (512G)

	__asm__ volatile("msr TCR_EL1, %0" ::"r"(tcr));

	__asm__ volatile("IC IALLU");
	__asm__ volatile("ISB");
	__asm__ volatile("TLBI VMALLE1");
	__asm__ volatile("DSB ISH");
	__asm__ volatile("ISB");

	volatile uint64_t sctlr = 0;
	__asm__ volatile("MRS %0, sctlr_el1"
					 : "=r"(sctlr));

	sctlr &= ~((1 << 25) | (1 << 24) | (1 << 19) | (1 << 12) | (1 << 4) | (1 << 3) | (1 << 2) | (1 << 1));
	sctlr |= (1 << 0);

	__asm__ volatile("MSR sctlr_el1, %0" ::"r"(sctlr));
	__asm__ volatile("ISB");
}