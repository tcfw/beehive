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

DEFINE_SYSCALL4(syscall_mem_map, SYSCALL_MEM_MAP, uintptr_t, paddr, uintptr_t, vaddr, size_t, length, int, flags)
{
	// terminal_logf("MM: addr=0x%X, len=0x%X, flags=0x%X", addr, length, flags);

	uintptr_t mapaddr = vaddr;
	size_t maplength = length;
	vm_mapping *existingMap = 0;

	// align length to page
	if (maplength % PAGE_SIZE != 0)
		maplength += PAGE_SIZE - (maplength % PAGE_SIZE);

	if (paddr % PAGE_SIZE != 0)
		return -ERRINVAL;

	if (mapaddr != 0)
	{
		// TODO(tcfw) check reservation -> actual allocation
		if (mapaddr % PAGE_SIZE != 0)
			return -ERRINVAL;

		existingMap = has_mapping(thread, mapaddr, maplength);
		if (existingMap != 0)
		{
			if (existingMap->page != 0 || existingMap->phy_addr != 0)
				return existingMap->vm_addr;
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
	if ((flags & MMAP_DEVICE) != 0)
		mapflags |= MEMORY_TYPE_DEVICE | VM_MAP_FLAG_DEVICE;

	if ((flags & MMAP_READ) == 0)
		mapflags |= MEMORY_PERM_RO;
	if ((flags & MMAP_WRITE) != 0)
		mapflags |= MEMORY_PERM_W;

	if (vaddr == 0 && existingMap == 0)
		existingMap = has_mapping(thread, mapaddr, maplength);

	if (paddr != 0)
	{
		// terminal_logf("mapping phy 0x%X to 0x%X len=%d", paddr, mapaddr, maplength);

		vm_mapping *mapping = kmalloc(sizeof(*mapping));
		if (mapping == 0)
			return -ERRNOMEM;

		int ret = vm_map_region(thread->process->vm.vm_table, paddr, mapaddr, maplength - 1, mapflags);
		if (ret < 0)
			return ret;

		mapping->flags = mapflags;
		mapping->length = maplength;
		mapping->page = 0;
		mapping->phy_addr = paddr;
		mapping->vm_addr = mapaddr;

		spinlock_acquire(&thread->process->lock);

		list_add_tail(&mapping->list, &thread->process->vm.vm_maps);
		thread->process->vm.brk = mapaddr + maplength;

		spinlock_release(&thread->process->lock);

		return mapaddr;
	}

	if (existingMap != NULL)
	{
		// terminal_logf("matching map addr=0x%X len=0x%X map_addr=0x%X map_len=0x%X", mapaddr, maplength, existingMap->vm_addr, existingMap->length);

		if (flags == 0 && (existingMap->flags & VM_MAP_FLAG_LAZY) == 0)
			return -ERREXISTS;

		if (existingMap->page == 0 && (existingMap->flags & VM_MAP_FLAG_LAZY) != 0)
			return actualise_lazy_reservation(thread, existingMap, mapflags);

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
	mapping->phy_addr = paddr;
	mapping->vm_addr = mapaddr;

	spinlock_acquire(&thread->process->lock);

	list_add_tail(&mapping->list, &thread->process->vm.vm_maps);
	thread->process->vm.brk = mapaddr + maplength;

	spinlock_release(&thread->process->lock);

	// terminal_logf("mapped addr 0x%X, len 0x%X, mapflags 0x%X", mapaddr, maplength, mapflags);

	return actualise_lazy_reservation(thread, mapping, mapflags);
}