#include <errno.h>
#include <kernel/clock.h>
#include <kernel/cls.h>
#include <kernel/context.h>
#include <kernel/list.h>
#include <kernel/mm.h>
#include <kernel/queue.h>
#include <kernel/sched.h>
#include <kernel/signal.h>
#include <kernel/strings.h>
#include <kernel/syscall.h>
#include <kernel/thread.h>
#include <kernel/tty.h>
#include <kernel/uaccess.h>
#include <kernel/vm.h>

static LIST_HEAD(threads);
static spinlock_t threads_lock;

static LIST_HEAD(procs);
static spinlock_t procs_lock;

static process_t kthreads_proc;

void init_proc(process_t *proc, char *cmd)
{
	spinlock_init(&proc->lock);

	INIT_LIST_HEAD(&proc->queues);
	INIT_LIST_HEAD(&proc->threads);
	INIT_LIST_HEAD(&proc->children);
	INIT_LIST_HEAD(&proc->vm.vm_maps);

	strncpy(proc->cmd, cmd, CMD_MAX);
	proc->state = STOPPED;
	proc->exitCode = -1;
	proc->nexttid = 1;

	proc->vm.vm_table = (vm_table *)page_alloc_s(sizeof(vm_table));
	vm_init_table(proc->vm.vm_table);

	process_list_entry_t *le = kmalloc(sizeof(process_list_entry_t));
	le->process = proc;

	spinlock_acquire(&procs_lock);
	list_add_tail(&le->list, &procs);
	spinlock_release(&procs_lock);
}

void init_kthread_proc()
{
	kthreads_proc.pid = 0;
	kthreads_proc.uid = 0;
	kthreads_proc.euid = 0;
	kthreads_proc.gid = 0;
	kthreads_proc.egid = 0;

	INIT_LIST_HEAD(&kthreads_proc.queues);
	INIT_LIST_HEAD(&kthreads_proc.vm.vm_maps);
	INIT_LIST_HEAD(&kthreads_proc.threads);

	strncpy(&kthreads_proc.cmd, "kthread", CMD_MAX);
	kthreads_proc.state = RUNNING;

	kthreads_proc.vm.vm_table = vm_get_kernel();

	populate_kernel_vm_maps(&kthreads_proc.vm);
}

void init_thread(thread_t *thread)
{
	init_context(&thread->ctx);

	thread->affinity = ~0;
	thread->sigactions.sig_stack.flags = SS_DISABLE;
	thread->tid = thread->process->nexttid;
	thread->process->nexttid++;

	struct clocksource_t *cs = clock_first(CS_GLOBAL);
	thread->timing.last_system = cs->val(cs);
	thread->timing.last_user = thread->timing.last_system;

	thread->sched_class = sched_get_class(SCHED_CLASS_LRF);

	thread_list_entry_t *entry = kmalloc(sizeof(thread_list_entry_t));
	entry->thread = thread;

	spinlock_acquire(&threads_lock);
	list_add_tail(&entry->list, &threads);
	spinlock_release(&threads_lock);
}

thread_t *create_kthread(void(entry)(void *), const char *name, void *data)
{
	thread_t *thread = (thread_t *)page_alloc_s(sizeof(thread_t));
	void *stack = (void *)page_alloc_s((size_t)KTHREAD_STACK_SIZE);

	thread->affinity = ~0;
	thread->flags = THREAD_KTHREAD;
	thread->process = &kthreads_proc;

	thread->tid = thread->process->nexttid;
	thread->process->nexttid++;

	strncpy(&thread->name, name, TNAME_MAX);

	thread->sched_class = sched_get_class(SCHED_CLASS_LRF);

	init_context(&thread->ctx);
	kthread_context(&thread->ctx, data);

	thread->ctx.pc = (uint64_t)entry;
	thread->ctx.sp = (uint64_t)stack;

	struct clocksource_t *cs = clock_first(CS_GLOBAL);
	thread->timing.last_system = cs->val(cs);

	thread_list_entry_t *tentry = kmalloc(sizeof(thread_list_entry_t));
	tentry->thread = thread;

	spinlock_acquire(&threads_lock);
	list_add_tail(&tentry->list, &threads);
	spinlock_release(&threads_lock);

	thread_list_entry_t *tpentry = kmalloc(sizeof(thread_list_entry_t));
	tpentry->thread = thread;

	list_add_tail(&tpentry->list, &kthreads_proc.threads);

	return thread;
}

struct list_head *get_threads()
{
	return &threads;
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
	struct thread_wait_cond_futex *futex_cond = (struct thread_wait_cond_futex *)thread->wc;

	if (futex_cond->timeout)
	{
		// terminal_logf("waking thread from futex timeout");
		if (futex_cond->timeout->timeout != 0)
		{
			// terminal_logf("after ts=0x%Xs 0x%Xns", futex_cond->timeout->timeout->seconds, futex_cond->timeout->timeout->nanoseconds);

			kfree(futex_cond->timeout->timeout);
			futex_cond->timeout->timeout = NULL;
		}
	}

	futex_hb_t *hb = futex_hb(&futex_cond->queue->key);
	spinlock_acquire(&hb->lock);
	list_del(&futex_cond->queue->list);
	kfree(futex_cond->queue);
	spinlock_release(&hb->lock);
}

void wake_thread(thread_t *thread)
{
	spinlock_acquire(&thread->wc_lock);

	if (thread->wc)
	{
		switch (thread->wc->type)
		{
		case SLEEP:
			thread_wake_from_sleep(thread);
			// terminal_logf("TID 0x%X:0x%X woke up from sleep", thread->process->pid, thread->tid);
			break;
		case QUEUE_IO:
			thread_wake_from_queue_io(thread);
			break;
		case WAIT:
			thread_wake_from_wait(thread);
			// terminal_logf("TID 0x%X:0x%X woke up from WAIT", thread->process->pid, thread->tid);
			break;
		}
		kfree(thread->wc);
		thread->wc = NULL;
	}

	set_thread_state(thread, THREAD_RUNNING);

	spinlock_release(&thread->wc_lock);

	cls_t *cls = get_core_cls(thread->running_core);
	thread->sched_class->enqueue_thread(&cls->rq, thread);
}

static int can_wake_thread_from_sleep(thread_t *thread)
{
	timespec_t ts;
	timespec_t d;

	struct thread_wait_cond_sleep *sleepcond = (struct thread_wait_cond_sleep *)thread->wc;
	struct clocksource_t *cs = clock_first(CS_GLOBAL);

	timespec_from_cs(cs, &ts);
	timespec_diff(&ts, &sleepcond->timer, &d);
	if (d.seconds >= 0 && d.nanoseconds >= 0)
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

static int can_wake_thread_from_wait(thread_t *thread)
{
	return 0; // only timeouts or futex_wake can wake
}

int can_wake_thread(thread_t *thread)
{
	if (thread->wc)
	{
		switch (thread->wc->type)
		{
		case SLEEP:
			return can_wake_thread_from_sleep(thread);
		case QUEUE_IO:
			return can_wake_thread_from_queue_io(thread);
		case WAIT:
			return can_wake_thread_from_wait(thread);
		}
	}

	return 1;
}

int sleep_thread(thread_t *thread, const timespec_t *ts, timespec_t *user_rem)
{
	struct clocksource_t *cs = clock_first(CS_GLOBAL);
	timespec_t sts;
	timespec_from_cs(cs, &sts);

	struct thread_wait_cond_sleep *sleep_cond = kmalloc(sizeof(struct thread_wait_cond_sleep));
	if (sleep_cond == NULL)
		return -ERRNOMEM;

	if (set_thread_state(thread, THREAD_SLEEPING) != THREAD_SLEEPING)
		return -ERRINVAL;

	sleep_cond->cond.type = SLEEP;
	sleep_cond->timer.seconds = sts.seconds + ts->seconds;
	sleep_cond->timer.nanoseconds = sts.nanoseconds + ts->nanoseconds;
	sleep_cond->user_rem = user_rem;

	thread->wc = sleep_cond;

	return 0;
}

void sleep_kthread(const timespec_t *ts, timespec_t *rem)
{
	syscall2(SYSCALL_NANOSLEEP, (uint64_t)ts, (uint64_t)rem);
}

void thread_wait_for_cond(thread_t *thread, const thread_wait_cond *cond)
{
	thread->wc = cond;
	set_thread_state(thread, THREAD_SLEEPING);
}

thread_t *get_first_thread_by_pid(pid_t pid)
{
	thread_list_entry_t *this;
	list_head_for_each(this, &threads)
	{
		if (this->thread->process->pid == pid)
			return this->thread;
	}

	return 0;
}

thread_t *get_current_sibling_thread_by_tid(tid_t tid)
{
	process_t *proc = current->process;
	spinlock_acquire(&proc->lock);
	thread_t *thread = 0;

	thread_list_entry_t *this;
	list_head_for_each(this, &proc->threads)
	{
		if (this->thread->tid == tid)
		{
			thread = this->thread;
			goto ret;
		}
	}

ret:
	spinlock_release(&proc->lock);
	return thread;
}

enum Thread_State set_thread_state(thread_t *thread, enum Thread_State state)
{
	memory_barrier;
	if (thread->state == THREAD_DEAD)
		return thread->state;

	thread->state = state;
	return state;
}

void mark_zombie_thread(thread_t *thread)
{
	set_thread_state(thread, THREAD_DEAD);

	// cls_t *cls = get_cls();
	// // thread->sched_class->dequeue_thread(&cls->rq, thread);

	// spinlock_acquire(&cls->sleepq.lock);

	// waitqueue_entry_t *this, *next;
	// list_head_for_each_safe(this, next, &cls->sleepq.head)
	// {
	// 	if (this->thread == thread)
	// 	{
	// 		// TODO(tcfw) clean up whatever is in *data
	// 		list_del(this);
	// 		kfree(this);
	// 	}
	// }

	// spinlock_release(&cls->sleepq.lock);
}

void free_process(process_t *proc)
{
	if (proc->vm.vm_table)
		page_free(proc->vm.vm_table);

	vm_mapping *vmm_cur, *vmm_next;
	list_head_for_each_safe(vmm_cur, vmm_next, &proc->vm.vm_maps)
	{
		page_free(vmm_cur->page);
		kfree(vmm_cur);
	}

	page_free(proc);
}

void free_thread(thread_t *thread)
{
	if (thread->wc)
	{
		// TODO(tcfw) actually clean up the wait cond
		kfree(thread->wc);
	}

	page_free(thread);
}