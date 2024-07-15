#include <kernel/arch.h>
#include <kernel/cls.h>
#include <kernel/mm.h>
#include <kernel/paging.h>
#include <kernel/panic.h>
#include <kernel/regions.h>
#include <kernel/strings.h>
#include <kernel/tty.h>
#include <kernel/vm.h>
#include <kernel/thread.h>
#include <gic.h>
#include "errno.h"

extern uintptr_t kernelstart;
extern uintptr_t kernelend;
extern uintptr_t kernelvstart;

extern void user_init(void);
extern uintptr_t address_xlate_read(uintptr_t offset);
extern uintptr_t address_xlate_write(uintptr_t offset);

static void vm_free_table_block(vm_table_block *block, int level);
static vm_table_block *vm_table_desc_to_block(uint64_t *desc);
static vm_table_block *vm_table_entry_to_block(uint64_t *entry);
static vm_table_block *vm_get_or_alloc_block(vm_table_block *parent, uint16_t entry);
static vm_table_block *vm_copy_link_table_block(uint64_t *parent, vm_table_block *tocopy);
static int vm_table_block_is_empty(vm_table_block *table);
static uintptr_t vm_va_to_pa_current(uintptr_t addr);
static uintptr_t vm_pa_to_va_current(uintptr_t addr);
uintptr_t vm_va_to_pa(vm_table *table, uintptr_t vptr);
uintptr_t vm_pa_to_va(vm_table *table, uintptr_t pptr);
uintptr_t vm_pa_to_kva(uintptr_t pptr);

vm_table *kernel_vm_map;

vm_table_block *vm_table_desc_to_block(uint64_t *desc)
{
	return (vm_table_block *)vm_pa_to_kva(*desc & VM_ENTRY_OA_MASK);
}

static vm_table_block *vm_table_entry_to_block(uint64_t *entry)
{
	return (vm_table_block *)vm_pa_to_kva(*entry & VM_ENTRY_OA_MASK);
}

vm_table *vm_get_current_table()
{
	thread_t *thread = current;
	if (thread)
	{
		vm_table *uvm = thread->process->vm.vm_table;
		if (uvm != 0)
			return uvm;
	}

	return kernel_vm_map;
}

vm_table *vm_get_kernel()
{
	return kernel_vm_map;
}

static uintptr_t __attribute__((noinline)) vm_va_to_pa_current(uintptr_t addr)
{
	__asm__ volatile("AT S1E1R, %0" ::"r"(addr));

	volatile uint64_t par;
	__asm__ volatile("MRS %0, PAR_EL1" : "=r"(par));

	if ((par & 0x1UL) == 1)
		return 0;

	uint64_t inpage = addr & 0xFFF;
	return (par & VM_ENTRY_OA_MASK) | inpage;
}

static uintptr_t vm_pa_to_va_current(uintptr_t addr)
{
	return vm_pa_to_va(vm_get_current_table(), addr);
}

uintptr_t vm_pa_to_va(vm_table *table, uintptr_t pptr)
{
	return 0;
}

uintptr_t vm_pa_to_kva(uintptr_t pptr)
{
	return pptr + ((uintptr_t)(&kernelvstart) - PHYS_START);
}

static int vm_table_block_is_empty(vm_table_block *table)
{
	int c = 0;

	for (int i = 0; i < 512; i++)
		if (table->entries[i] != 0)
		{
			c = 1;
			break;
		}

	return !c;
}

void vm_init_table(vm_table *table)
{
	memset(table, 0, sizeof(vm_table));
	vm_link_tables(table, kernel_vm_map);
}

void vm_set_kernel()
{
	vm_set_table(kernel_vm_map, 0);
}

uint64_t *vm_va_to_pte(vm_table *table, uintptr_t vptr)
{
	uint16_t l0 = vptr >> 39;
	uint16_t l1 = (vptr >> 30) & 0x1FF;
	uint16_t l2 = (vptr >> 21) & 0x1FF;
	uint16_t l3 = (vptr >> 12) & 0x1FF;

	if ((l0 > 511) | (l1 > 511) | (l2 > 511) | (l3 > 511))
	{
		return 0;
	}

	if (table->descriptors[l0] != 0)
	{
		vm_table_block *block = vm_table_desc_to_block(&table->descriptors[l0]);

		// l1
		if (block->entries[l1] == 0)
			return 0;

		if ((block->entries[l1] & VM_ENTRY_ISTABLE) == 0)
			return &block->entries[l1];

		// l2
		block = vm_table_entry_to_block(&block->entries[l1]);
		if (block->entries[l2] == 0)
			return 0;

		if ((block->entries[l2] & VM_ENTRY_ISTABLE) == 0)
			return &block->entries[l2];

		// l3
		block = vm_table_entry_to_block(&block->entries[l2]);
		if (block->entries[l3] == 0)
			return 0;

		if ((block->entries[l3] & VM_ENTRY_ISTABLE) != 0)
			return &block->entries[l3];
	}

	return 0;
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

	if (table->descriptors[l0] != 0)
	{
		vm_table_block *block = vm_table_desc_to_block(&table->descriptors[l0]);

		// l1
		if (block->entries[l1] == 0)
			return 0;

		if ((block->entries[l1] & VM_ENTRY_ISTABLE) == 0)
			return ((uintptr_t)(block->entries[l1] & 0x7FE0000000) | (vptr & 0x3FFFFFFFFF));

		// l2
		block = vm_table_entry_to_block(&block->entries[l1]);
		if (block->entries[l2] == 0)
			return 0;

		if ((block->entries[l2] & VM_ENTRY_ISTABLE) == 0)
			return ((uintptr_t)(block->entries[l2] & 0x7FFFF00000) | (vptr & 0xFFFFF));

		// l3
		block = vm_table_entry_to_block(&block->entries[l2]);
		if (block->entries[l3] == 0)
			return 0;

		if ((block->entries[l3] & VM_ENTRY_ISTABLE) != 0)
			return ((uintptr_t)(block->entries[l3] & 0x7FFFFFF000) | pageoff);
	}

	return 0;
}

static void vm_free_table_block(vm_table_block *block, int level)
{
	for (int i = 0; i < 512; i++)
	{
		if (
			(block->entries[i] != 0) &&
			(block->entries[i] & VM_ENTRY_MAPPED) == 0 &&
			(block->entries[i] & VM_ENTRY_ISTABLE) > 1 &&
			level < 3)
			vm_free_table_block(vm_table_entry_to_block(&block->entries[i]), level + 1);
	}
	page_free(block);
}

void vm_free_table(vm_table *table)
{
	for (int i = 0; i < 512; i++)
		if (
			(table->descriptors[i] != 0) &&
			(table->descriptors[i] & VM_DESC_LINKED) == 0)
			vm_free_table_block(vm_table_desc_to_block(&table->descriptors[i]), 1);

	page_free(table);
}

static vm_table_block *vm_get_or_alloc_block(vm_table_block *parent, uint16_t entry)
{
	if (entry > 511)
		return 0;

	vm_table_block *block;

	if (parent->entries[entry] == 0)
	{
		block = (vm_table_block *)page_alloc_s(sizeof(vm_table_block));
		memset(block, 0, sizeof(vm_table_block));

		parent->entries[entry] = vm_va_to_pa_current((uintptr_t)block) & VM_ENTRY_OA_MASK;
		parent->entries[entry] |= (VM_ENTRY_ISTABLE | VM_ENTRY_VALID | VM_ENTRY_NONSECURE);
	}
	else
		block = vm_table_entry_to_block(&parent->entries[entry]);

	return block;
}

int vm_unmap_region(vm_table *table, uintptr_t vstart, size_t size)
{
	uint64_t vend = vstart + (uintptr_t)size;

	// check vstart and vend is page aligned
	if ((vstart & 0xFFF) != 0 || (vend & 0xFFF) != 0xFFF)
	{
		terminal_logf("WARN: vm region map was not page aligned 0x%X:0x%X", vstart, vend);
		return -1;
	}

	uint16_t l0 = vstart >> 39;
	uint16_t l1 = (vstart >> 30) & 0x1FF;
	uint16_t l2 = (vstart >> 21) & 0x1FF;
	uint16_t l3 = (vstart >> 12) & 0x1FF;

	uint16_t l0_e = (vend >> 39);
	uint16_t l1_e = ((vend >> 30) & 0x1FF);
	uint16_t l2_e = ((vend >> 21) & 0x1FF);
	uint16_t l3_e = ((vend >> 12) & 0x1FF);

	uint16_t l0_d = l0_e - l0;
	uint16_t l1_d = l1_e - l1;
	uint16_t l2_d = l2_e - l2;

	int level = 0;
	int level_start = 0;
	int level_end = 0;

	if (l0_d != 0)
	{
		level = 0;
		level_start = l0;
		level_end = l0_e;
	}
	else if (l1_d != 0)
	{
		level = 1;
		level_start = l1;
		level_end = l1_e;
	}
	else if (l2_d != 0)
	{
		level = 2;
		level_start = l2;
		level_end = l2_e;
	}
	else
	{
		level = 3;
		level_start = l3;
		level_end = l3_e;
	}

	vm_table_block *table_l1 = 0;
	vm_table_block *table_l2 = 0;
	vm_table_block *table_l3 = 0;
	vm_table_block *cur_table = 0;

	int vstart_level = 0;

	uint64_t *desc = &table->descriptors[l0];
	if (*desc == 0)
		return 0;

	vstart_level++;
	table_l1 = vm_table_desc_to_block(desc);

	if ((table_l1->entries[l1] & VM_ENTRY_ISTABLE) != 0)
	{
		vstart_level++;
		table_l2 = vm_table_entry_to_block(&table_l1->entries[l1]);
	}

	if ((table_l2->entries[l2] & VM_ENTRY_ISTABLE) != 0)
	{
		vstart_level++;
		table_l3 = vm_table_entry_to_block(&table_l2->entries[l2]);
	}

	switch (level)
	{
	case 1:
		cur_table = table_l1;
		break;
	case 2:
		cur_table = table_l2;
		break;
	case 3:
		cur_table = table_l3;
		break;
	default:
		return -1;
	}

	for (int li = level_start; li <= level_end; li++)
	{
		if ((cur_table->entries[li] & VM_ENTRY_ISTABLE) != 0 && level != 3)
		{

			if (li == level_start)
			{
				terminal_logf("unmapping sublevel at start");
			}
			else if (li == level_end)
			{
				terminal_logf("unmapping sublevel at end");
			}
			else
			{
				terminal_logf("unmapping sublevel in middle");
				vm_free_table_block(vm_table_entry_to_block(&cur_table->entries[li]), level + 1);
				cur_table->entries[li] = 0;
			}
		}
		else
		{
			cur_table->entries[li] = 0;
		}

		// terminal_logf("unmapping L: 0x%x i: 0x%x sub-level: 0x%x", level, li, (cur_table->entries[li] & VM_ENTRY_ISTABLE) != 0);
	}

	int found_pte = 0;
	for (int i = 0; i < 512; i++)
	{
		if (cur_table->entries[i] != 0)
		{
			found_pte = 1;
			break;
		}
	}

	if (found_pte == 0)
	{
		// table is empty
		// terminal_logf("table is empty, freeing");
		vm_free_table_block(cur_table, level);
	}

	vm_clear_caches();

	return 0;
}

int vm_map_region(vm_table *table, uintptr_t pstart, uintptr_t vstart, size_t size, uint64_t flags)
{
	uint64_t vend = vstart + size;

	// check vstart and vend is page aligned
	if ((vstart & 0xFFF) != 0 || (vend & 0xFFF) != 0xFFF)
	{
		panicf("WARN: vm region map was not page aligned 0x%X:0x%X", vstart, vend);
		return -1;
	}

	uint16_t l0 = vstart >> 39;
	uint16_t l1 = (vstart >> 30) & 0x1FF;
	uint16_t l2 = (vstart >> 21) & 0x1FF;
	uint16_t l3 = (vstart >> 12) & 0x1FF;

	uint64_t *desc = &table->descriptors[l0];
	vm_table_block *table_l1 = 0;
	vm_table_block *table_l2 = 0;
	vm_table_block *table_l3 = 0;

	volatile uint64_t *vpage = 0;

	if (*desc == 0)
	{
		// alloc block
		table_l1 = (vm_table_block *)page_alloc_s(sizeof(vm_table_block));
		memset(table_l1, 0, sizeof(vm_table_block));

		*desc = (VM_DESC_VALID | VM_DESC_IS_DESC | VM_DESC_NONSECURE | VM_DESC_AF | VM_ENTRY_ISH);
		*desc |= vm_va_to_pa_current((uintptr_t)table_l1) & VM_DESC_NEXT_LEVEL_MASK;

		if (flags & MEMORY_TYPE_KERNEL)
			*desc |= VM_DESC_AP_KERNEL;

		// if (flags & MEMORY_PERM_RO)
		// 	*desc |= VM_DESC_AP_RO;

		if (flags & MEMORY_TYPE_DEVICE)
			*desc |= (1 << VM_DESC_ATTR) | VM_ENTRY_OSH;
	}
	else if ((*desc & VM_DESC_LINKED) != 0)
		table_l1 = vm_copy_link_table_block(desc, table_l1);
	else
		table_l1 = vm_table_desc_to_block(desc);

	while (vstart < vend)
	{
		int level = 3;
		int range = vend - vstart;
		int incsize = PAGE_SIZE;
		uint64_t addr = pstart & VM_ENTRY_OA_MASK;

		if (range > L1_BLOCK_SIZE && l3 == 0 && l2 == 0)
		{
			level = 1;
			incsize = L1_BLOCK_SIZE;
			vpage = &table_l1->entries[l1];
		}
		else if (range > L2_BLOCK_SIZE && l3 == 0)
		{
			if (!table_l2)
				table_l2 = vm_get_or_alloc_block(table_l1, l1);

			if (table_l2->entries[l2] & VM_ENTRY_LINKED)
				table_l2 = vm_copy_link_table_block(&table_l1->entries[l1], table_l2);

			level = 2;
			incsize = L2_BLOCK_SIZE;
			vpage = &table_l2->entries[l2];
		}
		else
		{
			if (!table_l2)
				table_l2 = vm_get_or_alloc_block(table_l1, l1);

			if (table_l2->entries[l2] & VM_ENTRY_LINKED)
				table_l2 = vm_copy_link_table_block(&table_l1->entries[l1], table_l2);

			if (!table_l3)
				table_l3 = vm_get_or_alloc_block(table_l2, l2);

			if (table_l3->entries[l3] & VM_ENTRY_LINKED)
				table_l3 = vm_copy_link_table_block(&table_l2->entries[l2], table_l3);

			vpage = &table_l3->entries[l3];
		}

		if ((*vpage & VM_ENTRY_MAPPED) > 0)
		{
			terminal_log("WARN: vm region map aleady mapped");
			return -2;
		}

		*vpage = addr | VM_ENTRY_VALID | VM_ENTRY_MAPPED | VM_ENTRY_NONSECURE | VM_ENTRY_AF | VM_ENTRY_ISH;
		// TODO(tcfw) check if contiguous - see armv8-a ref RCBXXM

		if ((flags & MEMORY_TYPE_DEVICE) != 0)
			*vpage |= (1 << VM_ENTRY_ATTR) | VM_ENTRY_OSH;
		else
			*vpage |= (0 << VM_ENTRY_ATTR);

		if (level == 3)
			*vpage |= VM_ENTRY_ISTABLE;

		if ((flags & MEMORY_TYPE_USER) != 0)
			*vpage |= VM_ENTRY_USER;

		if ((flags & MEMORY_NON_EXEC) != 0)
			*vpage |= VM_ENTRY_PXN | VM_ENTRY_UXN;

		if ((flags & MEMORY_USER_NON_EXEC) != 0)
			*vpage |= VM_ENTRY_UXN;

		if ((flags & MEMORY_PERM_RO) != 0)
			*vpage |= VM_ENTRY_PERM_RO | VM_ENTRY_PERM_W;

		if ((flags & MEMORY_PERM_W) != 0)
			*vpage |= VM_ENTRY_PERM_W;

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

		vstart += incsize;
		pstart += incsize;
	}

	return 0;
}

void vm_set_table(vm_table *table, pid_t pid)
{
	uintptr_t table_pa = vm_va_to_pa_current((uintptr_t)table);
	uint64_t asid = pid << TTBR_ASID_SHIFT & TTBR_ASID_MASK;
	uint64_t ttbr0 = asid | (table_pa & TTBR_BADDR_MASK) | 1;
	__asm__ volatile("MSR TTBR0_EL1, %0" ::"r"(ttbr0));

	// Enable E/S PAN
	uint64_t sctlr = 0;
	__asm__ volatile("MRS %0, SCTLR_EL1"
					 : "=r"(sctlr));
	sctlr |= 1ULL << 22 | 1ULL << 57; // EPAN | SPAN
	__asm__ volatile("MSR SCTLR_EL1, %0" ::"r"(sctlr));

	// invalidate TLBs
	vm_clear_caches();
}

void vm_clear_caches()
{
	__asm__ volatile("IC IALLU");
	__asm__ volatile("ISB");
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

	__asm__ volatile("msr TCR_EL1, %0" ::"r"(tcr));

	vm_clear_caches();

	uint64_t sctlr = 0;
	__asm__ volatile("MRS %0, sctlr_el1"
					 : "=r"(sctlr));

	sctlr &= ~((1 << 25) | (1 << 24) | (1 << 19) | (1 << 12) | (1 << 4) | (1 << 3) | (1 << 2) | (1 << 1));
	sctlr |= (1 << 0) | (1 << 12) | (1 << 2);

	__asm__ volatile("MSR sctlr_el1, %0" ::"r"(sctlr));
	__asm__ volatile("ISB");
}

static vm_table_block *vm_copy_link_table_block(uint64_t *parent, vm_table_block *tocopy)
{
	if ((*parent & VM_DESC_LINKED) == 0)
		return tocopy;

	vm_table_block *block = (vm_table_block *)page_alloc_s(sizeof(vm_table_block));

	for (int i = 0; i < 512; i++)
	{
		block->entries[i] = tocopy->entries[i];
		if (block->entries[i] != 0)
			block->entries[i] |= VM_ENTRY_LINKED;
	}

	*parent &= ~(VM_ENTRY_LINKED | VM_ENTRY_OA_MASK);
	*parent |= vm_va_to_pa_current((uintptr_t)block & VM_ENTRY_OA_MASK);

	return block;
}

int vm_link_tables(vm_table *table1, vm_table *table2)
{
	for (int i = 0; i < 512; i++)
	{
		if (table2->descriptors[i] == 0)
			continue;

		if (table1->descriptors[i] != 0)
			return -1;

		table1->descriptors[i] = table2->descriptors[i] | VM_DESC_LINKED;
	}

	return 0;
}

int access_ok(enum AccessType type, void *addr, size_t n)
{
	vm_table *cpt = vm_get_current_table();
	cls_t *cls = get_cls();

	uint64_t *vpage;

	if (addr == 0)
		return -ERRFAULT;

	while (n > 0)
	{
		vpage = vm_va_to_pte(cpt, (uintptr_t)addr);

		if (vpage == 0 || *vpage == 0)
		{
			return -ERRFAULT;
		}

		if ((*vpage & VM_ENTRY_USER) == 0 && (cls->rq.current_thread->flags & THREAD_KTHREAD) == 0)
		{
			return -ERRACCESS;
		}

		if (type == ACCESS_TYPE_WRITE && (*vpage & VM_ENTRY_PERM_RO) != 0)
		{
			return -ERRACCESS;
		}

		addr += PAGE_SIZE;
		if (__builtin_usubl_overflow(n, PAGE_SIZE, &n))
			break;
	}

	return 0;
}

void vm_init()
{
	kernel_vm_map = (vm_table *)page_alloc_s(sizeof(vm_table));

	// map terminal device space
	if (vm_map_region(kernel_vm_map, PHY_DEVICE_REGION, DEVICE_REGION, 4095, MEMORY_TYPE_KERNEL | MEMORY_TYPE_DEVICE) < 0)
		terminal_log("failed to map device region");

	// map GIC, GIC_CPU, GIC_REDIST regions
	// TODO(tcfw) get these regions from DBT
	if (vm_map_region(kernel_vm_map, GIC_DIST_BASE, DEVICE_REGION + GIC_DIST_BASE, 0x10000 - 1, MEMORY_TYPE_KERNEL | MEMORY_TYPE_DEVICE) < 0)
		terminal_log("failed to map GIV dist region");

	if (vm_map_region(kernel_vm_map, GIC_CPU_BASE, DEVICE_REGION + GIC_CPU_BASE, 0x10000 - 1, MEMORY_TYPE_KERNEL | MEMORY_TYPE_DEVICE) < 0)
		terminal_log("failed to map GIC cpu region");

	if (vm_map_region(kernel_vm_map, GIC_REDIST_BASE, DEVICE_REGION + GIC_REDIST_BASE, 0xf60000 - 1, MEMORY_TYPE_KERNEL | MEMORY_TYPE_DEVICE) < 0)
		terminal_log("failed to map GIC cpu region");

	// map kernel code & remaining physical memory regions
	if (vm_map_region(kernel_vm_map, (uintptr_t)ram_start(), (uintptr_t)ram_start() + VIRT_OFFSET, (size_t)ram_size() - 1, MEMORY_TYPE_KERNEL) < 0)
		terminal_log("failed to map kernel code region");

	// move DBT to above RAM
	if (vm_map_region(kernel_vm_map, PHY_DEVICE_DESCRIPTOR_REGION, DEVICE_DESCRIPTOR_REGION, 0x100000 - 1, MEMORY_TYPE_KERNEL) < 0)
		terminal_log("failed to map dbt region");

	terminal_logf("Loaded kernel is mapping device region to 0x%X", vm_va_to_pa(kernel_vm_map, 7765300871168));

	terminal_log("Loaded kernel VM map");
}

void vm_init_post_enable()
{
	setGICAddr((void *)DEVICE_REGION + GIC_DIST_BASE, (void *)DEVICE_REGION + GIC_REDIST_BASE, (void *)DEVICE_REGION + GIC_CPU_BASE);
}

void populate_kernel_vm_maps(vm_t *kernel_vm)
{
	vm_mapping *dev_region = kmalloc(sizeof(vm_mapping));
	vm_mapping *gic_region = kmalloc(sizeof(vm_mapping));
	vm_mapping *gic_cpu_region = kmalloc(sizeof(vm_mapping));
	vm_mapping *gic_redist_region = kmalloc(sizeof(vm_mapping));
	vm_mapping *mem_region = kmalloc(sizeof(vm_mapping));
	vm_mapping *desc_region = kmalloc(sizeof(vm_mapping));

	dev_region->phy_addr = PHY_DEVICE_REGION;
	dev_region->vm_addr = DEVICE_REGION;
	dev_region->length = 4096;

	// TODO(tcfw): get these regions from DBT
	gic_region->phy_addr = GIC_DIST_BASE;
	gic_region->vm_addr = DEVICE_REGION + GIC_DIST_BASE;
	gic_region->length = 0x10000;

	gic_cpu_region->phy_addr = GIC_CPU_BASE;
	gic_cpu_region->vm_addr = DEVICE_REGION + GIC_CPU_BASE;
	gic_cpu_region->length = 0x10000;

	gic_redist_region->phy_addr = GIC_REDIST_BASE;
	gic_redist_region->vm_addr = DEVICE_REGION + GIC_REDIST_BASE;
	gic_redist_region->length = 0xf60000;

	mem_region->phy_addr = (uintptr_t)ram_start();
	mem_region->vm_addr = (uintptr_t)ram_start() + VIRT_OFFSET;
	mem_region->length = (size_t)ram_size() - 1;
	mem_region->flags = VM_MAP_FLAG_SHARED;

	desc_region->phy_addr = PHY_DEVICE_DESCRIPTOR_REGION;
	desc_region->vm_addr = DEVICE_DESCRIPTOR_REGION;
	desc_region->length = 0x100000 - 1;

	list_add(dev_region, &kernel_vm->vm_maps);
	list_add(gic_region, &kernel_vm->vm_maps);
	list_add(gic_cpu_region, &kernel_vm->vm_maps);
	list_add(gic_redist_region, &kernel_vm->vm_maps);
	list_add(mem_region, &kernel_vm->vm_maps);
	list_add(desc_region, &kernel_vm->vm_maps);
}

int current_vm_region_shared(uintptr_t uaddr, size_t len)
{
	vm_mapping *this;
	struct list_head *vm_maps = (struct list_head *)&current->process->vm.vm_maps;

	list_head_for_each(this, vm_maps)
	{
		if (!(this->vm_addr <= uaddr && this->vm_addr + this->length >= uaddr + len))
			continue;

		if (this->flags & VM_MAP_FLAG_SHARED)
		{
			return 1;
		}
	}

	return 0;
}
