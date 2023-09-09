#include <kernel/clock.h>
#include <kernel/context.h>
#include <kernel/mm.h>
#include <kernel/thread.h>

void init_thread(thread_t *thread)
{
	init_context(&thread->ctx);

	thread->affinity = ~0;

	struct clocksource_t *cs = clock_first(CS_GLOBAL);
	thread->timing.last_system = cs->val(cs);
	thread->timing.last_user = thread->timing.last_system;

	thread->vm_table = (vm_table *)page_alloc_s(sizeof(vm_table));
	vm_init_table(thread->vm_table);
}