#include <kernel/thread.h>

void arch_thread_prep_switch(thread_t *thread)
{
	__asm__ volatile("msr TPIDRRO_EL0, %0" ::"r"(thread->pid));
}