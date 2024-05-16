#include <errno.h>
#include <kernel/clock.h>
#include <kernel/cls.h>
#include <kernel/context.h>
#include <kernel/mm.h>
#include <kernel/queue.h>
#include <kernel/sched.h>
#include <kernel/strings.h>
#include <kernel/syscall.h>
#include <kernel/thread.h>
#include <kernel/uaccess.h>
#include <kernel/vm.h>

#define KTHREAD_STACK_SIZE (1024 * 1024)

static LIST_HEAD(threads);
static spinlock_t threads_lock;

static LIST_HEAD(procs);
static spinlock_t procs_lock;

static process_t kthreads_proc;

void init_kthread_proc() {
	INIT_LIST_HEAD(&kthreads_proc.shm);
	INIT_LIST_HEAD(&kthreads_proc.queues);
	INIT_LIST_HEAD(&kthreads_proc.vm.vm_maps);

	kthreads_proc.pid = 0;
	kthreads_proc.uid = 0;
	kthreads_proc.euid = 0;
	kthreads_proc.gid = 0;
	kthreads_proc.egid = 0;

	strcpy(kthreads_proc.cmd, "kthread");

	kthreads_proc.vm.vm_table = vm_get_kernel();
	INIT_LIST_HEAD(&kthreads_proc.vm.vm_maps);

	populate_kernel_vm_maps(&kthreads_proc.vm);
}

void init_thread(thread_t *thread)
{
	init_context(&thread->ctx);

	thread->affinity = ~0;

	struct clocksource_t *cs = clock_first(CS_GLOBAL);
	thread->timing.last_system = cs->val(cs);
	thread->timing.last_user = thread->timing.last_system;

	thread->process->vm.vm_table = (vm_table *)page_alloc_s(sizeof(vm_table));
	vm_init_table(thread->process->vm.vm_table);

	thread->sched_class = sched_get_class(SCHED_CLASS_LRF);

	thread_list_entry_t *entry = (thread_list_entry_t *)kmalloc(sizeof(thread_list_entry_t));
	entry->thread = thread;

	spinlock_acquire(&threads_lock);
	list_add_tail(&threads, entry);
	spinlock_release(&threads_lock);
}

thread_t *create_kthread(void(entry)(void *), const char *name, void *data)
{
	thread_t *thread = (thread_t *)page_alloc_s(sizeof(thread_t));
	void *stack = (void *)page_alloc_s((size_t)KTHREAD_STACK_SIZE);

	thread->affinity = ~0;
	thread->flags = THREAD_KTHREAD;
	thread->process = &kthreads_proc;
	strcpy(&thread->name, name);

	thread->sched_class = sched_get_class(SCHED_CLASS_LRF);

	init_context(&thread->ctx);
	kthread_context(&thread->ctx, data);

	thread->ctx.pc = (uint64_t)entry;
	thread->ctx.sp = (uint64_t)stack;

	struct clocksource_t *cs = clock_first(CS_GLOBAL);
	thread->timing.last_system = cs->val(cs);

	thread_list_entry_t *tentry = (thread_list_entry_t *)kmalloc(sizeof(thread_list_entry_t));
	tentry->thread = thread;

	spinlock_acquire(&threads_lock);
	list_add_tail(tentry, &threads);
	spinlock_release(&threads_lock);

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

		thread_return_wc(thread, (void *)-ERRINTR);
	}
}

static void thread_wake_from_queue_io(thread_t *thread)
{
}

static void thread_wake_from_wait(thread_t *thread)
{

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
			thread_wake_from_queue_io(thread);
			break;
		case WAIT:
			thread_wake_from_wait(thread);
			break;
		}

		kfree(thread->wc);
		thread->wc = NULL;
	}

	thread->state = THREAD_RUNNING;
}

static int can_wake_thread_from_sleep(thread_t *thread)
{
	struct thread_wait_cond_sleep *sleepcond = (struct thread_wait_cond_sleep *)thread->wc;
	struct clocksource_t *cs = clock_first(CS_GLOBAL);
	timespec_t ts;
	timespec_t d;
	timespec_from_cs(cs, &ts);
	timespec_diff(&ts, &sleepcond->timer, &d);
	if (d.seconds >= 0)
		return 1;
	return 0;
}

static int can_wake_thread_from_queue_io(thread_t *thread)
{
	queue_ref_t *queue = NULL;
	struct thread_wait_cond_queue_io *wc = (struct thread_wait_cond_queue_io *)thread->wc;

	list_head_for_each(queue, &wc->queues)
	{
		uint64_t c = list_len(&queue->queue->buffer);
		if ((wc->flags | THREAD_QUEUE_IO_WRITE) != 0)
		{
			if (c < queue->queue->max_msg_count)
				return 1;
		}
		else
		{
			if (c > 0)
				return 1;
		}
	}

	return 0;
}

int can_wake_thread(thread_t *thread)
{
	if (thread->wc)
	{
		switch (thread->wc->type)
		{
		case SLEEP:
			return can_wake_thread_from_sleep(thread);
			break;
		case QUEUE_IO:
			return can_wake_thread_from_queue_io(thread);
			break;
		}

		kfree(thread->wc);
	}

	return 1;
}

void sleep_thread(thread_t *thread, const timespec_t *ts, timespec_t *user_rem)
{
	struct clocksource_t *cs = clock_first(CS_GLOBAL);
	timespec_t sts;
	timespec_from_cs(cs, &sts);

	struct thread_wait_cond_sleep *sleep_cond = (struct thread_wait_cond_sleep *)kmalloc(sizeof(struct thread_wait_cond_sleep));

	sleep_cond->cond.type = SLEEP;
	sleep_cond->timer.seconds = sts.seconds + ts->seconds;
	sleep_cond->timer.nanoseconds = sts.nanoseconds + ts->nanoseconds;
	sleep_cond->user_rem = user_rem;

	thread->wc = sleep_cond;
	thread->state = THREAD_SLEEPING;
}

void sleep_kthread(const timespec_t *ts, timespec_t *rem)
{
	syscall2(SYSCALL_NANOSLEEP, (uint64_t)ts, (uint64_t)rem);
}

void thread_wait_for_cond(thread_t *thread, const thread_wait_cond *cond)
{
	thread->wc = cond;
	thread->state = THREAD_SLEEPING;
}

thread_t *get_thread_by_pid(pid_t pid)
{
	thread_list_entry_t *this;
	list_head_for_each(this, &threads)
	{
		if (this->thread->process->pid == pid && this->thread->tid == (tid_t)pid)
		{
			return this->thread;
		}
	}

	return 0;
}

void mark_zombie_thread(thread_t *thread)
{
	thread->state = ZOMBIE;

	cls_t *cls = get_cls();
	thread->sched_class->dequeue_thread(&cls->rq, thread);
}