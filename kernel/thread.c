#include <kernel/clock.h>
#include <kernel/context.h>
#include <kernel/mm.h>
#include <kernel/thread.h>
#include <kernel/vm.h>
#include <kernel/strings.h>
#include <kernel/uaccess.h>
#include <errno.h>

#define KTHREAD_STACK_SIZE (1024 * 1024)

void init_thread(thread_t *thread)
{
	INIT_LIST_HEAD(&thread->shm);
	INIT_LIST_HEAD(&thread->queues);
	INIT_LIST_HEAD(&thread->vm_maps);

	init_context(&thread->ctx);

	thread->affinity = ~0;

	struct clocksource_t *cs = clock_first(CS_GLOBAL);
	thread->timing.last_system = cs->val(cs);
	thread->timing.last_user = thread->timing.last_system;

	thread->vm_table = (vm_table *)page_alloc_s(sizeof(vm_table));
	vm_init_table(thread->vm_table);

	thread->sched_class = sched_get_class(SCHED_CLASS_LRF);
}

thread_t *create_kthread(void *(entry)(void), const char *name, void *data)
{
	thread_t *thread = (thread_t *)page_alloc_s(sizeof(thread_t));
	void *stack = (void *)page_alloc_s((size_t)KTHREAD_STACK_SIZE);

	strcpy(thread->cmd, name);

	thread->affinity = ~0;
	thread->flags = THREAD_KTHREAD;

	thread->pid = 0;
	thread->uid = 0;
	thread->euid = 0;
	thread->gid = 0;
	thread->egid = 0;

	thread->sched_class = sched_get_class(SCHED_CLASS_LRF);

	init_context(&thread->ctx);
	kthread_context(&thread->ctx, data);

	thread->ctx.pc = (uint64_t)entry;
	thread->ctx.sp = (uint64_t)stack;

	thread->vm_table = vm_get_kernel();

	struct clocksource_t *cs = clock_first(CS_GLOBAL);
	thread->timing.last_system = cs->val(cs);

	return thread;
}

static void thread_wake_from_sleep(thread_t *thread)
{
	struct thread_wait_cond_sleep *sleep_cond = (struct thread_wait_cond_sleep *)thread->wc;
	struct clocksource_t *cs = clock_first(CS_GLOBAL);

	timespec_t now, rem;
	timespec_from_cs(cs, &now);
	timespec_diff(&sleep_cond->timer, &now, &rem);

	if (rem.seconds < 0)
		thread_return_wc(thread, 0);
	else
	{
		if (sleep_cond->user_rem != 0)
			copy_to_user(&rem, sleep_cond->user_rem, sizeof(timespec_t));

		thread_return_wc(thread, -ERRINTR);
	}
}

void wake_thread(thread_t *thread)
{
	if (thread->wc)
	{
		switch (thread->wc->type)
		{
		case SLEEP:
			thread_wake_from_sleep(thread);
			break;
		case QUEUE_IO:
			break;
		}

		kfree(thread->wc);
	}

	thread->state = RUNNING;
}

void sleep_thread(thread_t *thread, const timespec_t *ts, timespec_t *user_rem)
{
	struct thread_wait_cond_sleep *sleep_cond = (struct thread_wait_cond_sleep *)page_alloc_s(sizeof(struct thread_wait_cond_sleep));

	sleep_cond->cond.type = SLEEP;
	sleep_cond->timer.seconds = ts->seconds;
	sleep_cond->timer.nanoseconds = ts->nanoseconds;
	sleep_cond->user_rem = user_rem;

	thread->wc = sleep_cond;
	thread->state = SLEEPING;
}