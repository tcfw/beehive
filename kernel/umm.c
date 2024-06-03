#include <kernel/cls.h>
#include <kernel/signal.h>
#include <kernel/thread.h>
#include <kernel/tty.h>
#include <kernel/umm.h>

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

void user_data_abort(uintptr_t daddr, enum UserDataAbortOp op, uintptr_t pc)
{
	thread_t *thread = current;

	terminal_logf("user data abort on TID (0x%x:0x%x): request=0x%X wnr=%d PC=0x%X", thread->process->pid, thread->tid, daddr, op, pc);

	vm_mapping *map = has_mapping(thread, daddr, 8);
	if (map == 0)
		send_signal(thread, SIG_SEGV);

	return;
}