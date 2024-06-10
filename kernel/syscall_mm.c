#include <errno.h>
#include <kernel/list.h>
#include <kernel/mm.h>
#include <kernel/mmap.h>
#include <kernel/paging.h>
#include <kernel/panic.h>
#include <kernel/regions.h>
#include <kernel/stdint.h>
#include <kernel/strings.h>
#include <kernel/syscall.h>
#include <kernel/tty.h>
#include <kernel/umm.h>
#include <kernel/unistd.h>
#include <kernel/vm.h>

static uint64_t lazy_mem_map(thread_t *thread, uintptr_t addr, size_t length, int flags)
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

static int64_t actualise_reservation(thread_t *thread, vm_mapping *map, int flags)
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

	uintptr_t pa = vm_va_to_pa(thread->process->vm.vm_table, (uintptr_t)page);

	spinlock_acquire(&thread->process->lock);

	map->phy_addr = pa;
	map->page = page;
	map->flags = flags;

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
		ret = actualise_reservation(thread, split, flags);
		if (ret < 0)
			return ret;
	}

	ret = map->vm_addr;

out:
	spinlock_release(&thread->process->lock);
	return ret;
}

DEFINE_SYSCALL2(syscall_mem_unmap, SYSCALL_MEM_UMAP, uintptr_t, addr, size_t, length)
{
	// terminal_logf("MUNM: addr=0x%X length=0x%X", addr, length);

	int unmapAddr = addr;
	if (unmapAddr < thread->process->vm.start_brk)
		unmapAddr = thread->process->vm.start_brk;

	spinlock_acquire(&thread->process->lock);

	vm_mapping *this, *next;
	list_head_for_each_safe(this, next, &thread->process->vm.vm_maps)
	{
		if (this->vm_addr < unmapAddr || this->vm_addr > (unmapAddr + length))
			continue;

		// terminal_logf("unmapping addr 0x%X", this->vm_addr);

		// TODO(tcfw) free shared pages when references go to zero
		if (this->page != 0 && this->phy_addr == 0 && (this->flags & VM_MAP_FLAG_SHARED) == 0)
		{
			// terminal_log("and freed pages");
			vm_unmap_region(thread->process->vm.vm_table, this->vm_addr, this->length - 1);
			page_free(this->page);
		}

		list_del(&this->list);
		kfree(this);
	}

	spinlock_release(&thread->process->lock);

	return 0;
}

DEFINE_SYSCALL3(syscall_mem_map, SYSCALL_MEM_MAP, uintptr_t, addr, size_t, length, int, flags)
{
	// terminal_logf("MM: addr=0x%X, len=0x%X, flags=0x%X", addr, length, flags);

	uintptr_t mapaddr = addr;
	size_t maplength = length;
	vm_mapping *existingMap = 0;

	// align length to page
	if (maplength % PAGE_SIZE != 0)
		maplength += PAGE_SIZE - (maplength % PAGE_SIZE);

	if (mapaddr != 0)
	{
		// TODO(tcfw) check reservation -> actual allocation
		if (mapaddr % PAGE_SIZE != 0)
			return -ERRINVAL;

		existingMap = has_mapping(thread, mapaddr, maplength);
		if (existingMap != 0)
		{
			if (existingMap->page != 0 || existingMap->phy_addr != 0)
				return -ERREXISTS;
		}
		else if (mapaddr < thread->process->vm.brk)
			return -ERRINVAL;
	}
	else
		mapaddr = thread->process->vm.brk;

	if (mapaddr % PAGE_SIZE != 0)
		mapaddr += PAGE_SIZE - (mapaddr % PAGE_SIZE);

	if ((mapaddr + maplength) > VIRT_OFFSET)
		return -ERRINVAL;

	int mapflags = MEMORY_TYPE_USER;

	if ((flags & MMAP_READ) == 0)
		mapflags |= MEMORY_PERM_RO;
	if ((flags & MMAP_WRITE) != 0)
		mapflags |= MEMORY_PERM_W;

	if (addr == 0 && existingMap == 0)
		existingMap = has_mapping(thread, mapaddr, maplength);

	if (existingMap != NULL)
	{
		// terminal_logf("matching map addr=0x%X len=0x%X map_addr=0x%X map_len=0x%X", mapaddr, maplength, existingMap->vm_addr, existingMap->length);

		if (flags == 0 || (existingMap->flags & VM_MAP_FLAG_LAZY) == 0)
			return -ERREXISTS;

		if (existingMap->page == 0 && (existingMap->flags & VM_MAP_FLAG_LAZY) != 0)
			return actualise_reservation(thread, existingMap, mapflags);

		return -ERRINVAL;
	}

	if (maplength > VM_MAX_IMMD_ALLOC || flags == 0)
		return lazy_mem_map(thread, mapaddr, maplength, mapflags);

	vm_mapping *mapping = kmalloc(sizeof(*mapping));
	if (mapping == 0)
		return -ERRNOMEM;

	mapping->flags = mapflags;
	mapping->length = maplength;
	mapping->page = 0;
	mapping->phy_addr = 0;
	mapping->vm_addr = mapaddr;

	spinlock_acquire(&thread->process->lock);

	list_add_tail(&mapping->list, &thread->process->vm.vm_maps);
	thread->process->vm.brk = mapaddr + maplength;

	spinlock_release(&thread->process->lock);

	// terminal_logf("mapped addr 0x%X, len 0x%X, flags 0x%X", mapaddr, maplength, flags);

	return actualise_reservation(thread, mapping, mapflags);
}