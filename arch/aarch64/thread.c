#include <kernel/thread.h>
#include <kernel/vm.h>

void arch_thread_prep_switch(thread_t *thread)
{
	__asm__ volatile("msr TPIDRRO_EL0, %0" ::"r"(thread->pid));
}

void thread_return_wc(thread_t *thread, void *data1)
{
	thread->ctx.regs[0] = (uint64_t)data1;
}