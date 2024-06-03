#include <errno.h>
#include <kernel/cls.h>
#include <kernel/irq.h>
#include <kernel/list.h>
#include <kernel/mm.h>
#include <kernel/sched.h>
#include <kernel/sync.h>
#include <kernel/syscall.h>
#include <kernel/thread.h>
#include <kernel/tty.h>
#include <kernel/uaccess.h>

DEFINE_SYSCALL2(syscall_sched_getaffinity, SYSCALL_SCHED_GETAFFINITY, pid_t, pid, uint64_t *, affinity)
{
	int access = access_ok(ACCESS_TYPE_WRITE, affinity, sizeof(uint64_t));
	if (access < 0)
		return access;

	thread_t *curthread = get_thread_by_pid(pid);
	if (curthread == 0)
		return -ERRNOPROC;

	copy_to_user(&curthread->affinity, affinity, sizeof(uint64_t));

	return 0;
}

DEFINE_SYSCALL2(syscall_sched_setaffinity, SYSCALL_SCHED_SETAFFINITY, pid_t, pid, const uint64_t *, affinity)
{
	return -ERRNOSYS;
}

DEFINE_SYSCALL1(syscall_exit_group, SYSCALL_EXIT_GROUP, int, code)
{
	process_t *proc = thread->process;
	mark_zombie_thread(thread);

	spinlock_acquire(&proc->lock);

	if (proc->exitCode == -1)
	{
		proc->exitCode = code;
		proc->state = ZOMBIE;
	}

	// leave threads for reaping
	thread_list_entry_t *this, *next;
	list_head_for_each_safe(this, next, &proc->threads)
	{
		if (this->thread == thread)
			continue;

		int core = thread_is_running(this->thread);
		if (core != -1)
		{
			cls_t *cls = get_core_cls(core);
			int state = spinlock_acquire_irq(&cls->rq.lock);
			mark_zombie_thread(this->thread);
			spinlock_release_irq(state, &cls->rq.lock);
			send_soft_irq(core, SOFT_IRQ_THREAD_STOP);
		}
		else
			mark_zombie_thread(this->thread);
	}

	spinlock_release(&proc->lock);

	terminal_logf("PID %d exit: %d", proc->pid, proc->exitCode);
}

DEFINE_SYSCALL1(syscall_exit, SYSCALL_EXIT, int, code)
{
	set_thread_state(current, THREAD_DEAD);

	terminal_logf("thread ended TID=0x%x:0x%x", thread->process->pid, thread->tid);

	return 0;
}

DEFINE_SYSCALL3(syscall_thread_start, SYSCALL_THREAD_START, void *, func, void *, stack, void *, arg)
{
	int access = access_ok(ACCESS_TYPE_READ, func, 1);
	if (access < 0)
		return access;

	access = access_ok(ACCESS_TYPE_WRITE, stack, 1);
	if (access < 0)
		return access;

	access = access_ok(ACCESS_TYPE_READ, arg, 1);
	if (access < 0)
		return access;

	thread_t *newthread = (thread_t *)page_alloc_s(sizeof(thread_t));
	if (!newthread)
	{
		return -ERRNOMEM;
	}

	newthread->process = current->process;
	init_thread(newthread);
	newthread->ctx.pc = (uint64_t)func;
	newthread->ctx.sp = (uintptr_t)stack;
	newthread->ctx.regs[0] = (uint64_t)arg;

	thread_list_entry_t *tle = kmalloc(sizeof(thread_list_entry_t));
	if (!tle)
	{
		free_thread(newthread);
		return -ERRNOMEM;
	}

	tle->thread = newthread;
	list_add_tail(&tle->list, &current->process->threads);

	set_thread_state(thread, THREAD_RUNNING);
	sched_append_pending(newthread);

	terminal_logf("added new thread TID=0x%x:0x%x", newthread->process->pid, newthread->tid);

	return newthread->tid;
}