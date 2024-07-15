#include "errno.h"
#include <kernel/cls.h>
#include <kernel/list.h>
#include <kernel/mm.h>
#include <kernel/signal.h>
#include <kernel/strings.h>
#include <kernel/thread.h>
#include <kernel/tty.h>
#include <kernel/umm.h>
#include <kernel/vm.h>

#define LAZY_MAX_ALLOC_SIZE (65536)

uint64_t lazy_mem_map(thread_t *thread, uintptr_t addr, size_t length, int flags)
{
	vm_mapping *map = kmalloc(sizeof(*map));
	if (map == 0)
		return -ERRNOMEM;

	map->flags = flags | VM_MAP_FLAG_LAZY;
	map->length = length;
	map->page = 0;
	map->phy_addr = 0;
	map->vm_addr = addr;

	spinlock_acquire(&thread->process->lock);

	list_add_tail(&map->list, &thread->process->vm.vm_maps);
	thread->process->vm.brk = addr + length;

	spinlock_release(&thread->process->lock);

	// terminal_logf("LZMM: addr 0x%X, len 0x%X, flags 0x%X", addr, length, flags);

	return addr;
}

int64_t actualise_lazy_reservation(thread_t *thread, vm_mapping *map, int flags)
{
	uint64_t wantSize = map->length;
	uint64_t maplength = map->length;
	void *page = NULL;
	int64_t ret = 0;

	// terminal_logf("actualising addr=0x%X len=0x%X", map->vm_addr, map->length);

	while (wantSize > 0)
	{
		page = page_alloc_s(wantSize);
		if (page != 0)
			break;

		if (wantSize <= PAGE_SIZE)
			break;

		wantSize >>= 1;
		wantSize += PAGE_SIZE - (wantSize % PAGE_SIZE);
	}

	if (page == NULL)
		return -ERRNOMEM;

	maplength -= wantSize;
	memset(page, 0, wantSize);

	uintptr_t pa = vm_va_to_pa(vm_get_current_table(), (uintptr_t)page);

	spinlock_acquire(&thread->process->lock);

	map->phy_addr = pa;
	map->page = page;
	map->flags = flags & ~(VM_MAP_FLAG_LAZY);

	ret = vm_map_region(thread->process->vm.vm_table, pa, map->vm_addr, wantSize - 1, map->flags);
	if (ret < 0)
		goto out;

	if (maplength > 0)
	{
		// split mapping
		map->length = wantSize;

		vm_mapping *split = kmalloc(sizeof(*split));
		if (split == 0)
		{
			ret = -ERRNOMEM;
			goto out;
		}

		split->flags = map->flags;
		split->length = wantSize;
		split->page = 0;
		split->phy_addr = 0;
		split->vm_addr = map->vm_addr + wantSize;

		list_add_after(&split->list, &map->list);

		spinlock_release(&thread->process->lock);
		ret = actualise_lazy_reservation(thread, split, flags);
		if (ret < 0)
			return ret;
	}
	vm_clear_caches();

	ret = map->vm_addr;

out:
	spinlock_release(&thread->process->lock);
	return ret;
}

vm_mapping *has_mapping(thread_t *thread, uintptr_t addr, size_t length)
{
	uintptr_t mapTop = addr + length;
	vm_mapping *this = NULL;
	uintptr_t top = 0;

	spinlock_acquire(&thread->process->lock);

	list_head_for_each(this, &thread->process->vm.vm_maps)
	{
		top = this->vm_addr + this->length;

		if (this->vm_addr <= addr && top >= mapTop)
		{
			spinlock_release(&thread->process->lock);
			return this;
		}
	}

	spinlock_release(&thread->process->lock);
	return 0;
}

int vm_alloc_lazy_mapping(thread_t *thread, vm_mapping *map, uint64_t alloc_size)
{

	if (alloc_size < map->length && (map->length - alloc_size) > PAGE_SIZE)
	{
		spinlock_acquire(&thread->process->lock);
		// split
		vm_mapping *sibmap = kmalloc(sizeof(*map));
		if (sibmap == 0)
		{
			spinlock_release(&thread->process->lock);
			return -ERRNOMEM;
		}

		sibmap->flags = map->flags;
		sibmap->phy_addr = map->phy_addr + alloc_size;
		sibmap->vm_addr = map->vm_addr + alloc_size;
		sibmap->length = map->length - alloc_size;
		sibmap->page = 0;
		list_add_after(&sibmap->list, &map->list);

		map->length = alloc_size;
		spinlock_release(&thread->process->lock);
	}

	return actualise_lazy_reservation(thread, map, map->flags);
}

void user_data_abort(const uintptr_t daddr, enum UserDataAbortOp op, uintptr_t pc)
{
	thread_t *thread = current;

	terminal_logf("user data abort on TID (0x%x:0x%x): request=0x%X wnr=%d PC=0x%X", thread->process->pid, thread->tid, daddr, op, pc);

	vm_mapping *map = has_mapping(thread, daddr, 8);
	if (map == 0)
		send_signal(thread, SIG_SEGV);
	else if ((map->flags | VM_MAP_FLAG_LAZY) > 0 && map->page == 0 && map->phy_addr == 0)
	{
		int ret = vm_alloc_lazy_mapping(thread, map, LAZY_MAX_ALLOC_SIZE);
		if (ret <= 0)
			terminal_logf("failed to actualise lazy reservation on TID (0x%x:0x%x): request=0x%X wnr=%d PC=0x%X ~> ret=%d", thread->process->pid, thread->tid, daddr, op, pc, ret);
	}
	else
	{
		uint64_t *pte = vm_va_to_pte(thread->process->vm.vm_table, daddr);
		if (pte != 0)
			terminal_logf("PTE: 0x%X", pte);
		set_thread_state(thread, THREAD_STOPPED);
	}

	return;
}