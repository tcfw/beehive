#include <kernel/thread.h>
#include <kernel/vm.h>

void arch_thread_prep_switch(thread_t *thread)
{
	uint64_t pid = (uint64_t)thread->pid;
	__asm__ volatile("MSR TPIDRRO_EL0, %0" ::"r"(pid));
	return;
}

void thread_return_wc(thread_t *thread, void *data1)
{
	thread->ctx.regs[0] = (uint64_t)data1;
	return;
}