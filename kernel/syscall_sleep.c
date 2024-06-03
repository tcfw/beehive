#include <errno.h>
#include <kernel/cls.h>
#include <kernel/list.h>
#include <kernel/mm.h>
#include <kernel/sched.h>
#include <kernel/syscall.h>
#include <kernel/thread.h>
#include <kernel/uaccess.h>
#include <kernel/tty.h>
#include <kernel/wait.h>

DEFINE_SYSCALL2(syscall_nanosleep, SYSCALL_NANOSLEEP, const timespec_t *, uts, timespec_t *, remaining)
{
	timespec_t ts = {};

	int access = access_ok(ACCESS_TYPE_READ, uts, sizeof(timespec_t));
	if (access < 0)
		return access;

	if (remaining)
	{
		access = access_ok(ACCESS_TYPE_WRITE, remaining, sizeof(timespec_t));
		if (access < 0)
			return access;
	}

	int ret = copy_from_user(uts, &ts, sizeof(ts));
	if (ret < 0)
		return ret;

	cls_t *cls = get_cls();

	sleep_thread(thread, &ts, remaining);

	// terminal_logf("TID 0x%X:0x%X sleeping for 0x%Xs 0x%Xns", thread->process->pid, thread->tid, ts.seconds, ts.nanoseconds);

	waitqueue_entry_t *wqe = kmalloc(sizeof(waitqueue_entry_t));
	if (wqe == NULL)
	{
		kfree(thread->wc);
		set_thread_state(thread, THREAD_RUNNING);
		return -ERRNOMEM;
	}

	wqe->thread = thread;
	wqe->func = wq_can_wake_thread;

	spinlock_acquire(&cls->sleepq.lock);
	list_add_tail(&wqe->list, &cls->sleepq.head);
	spinlock_release(&cls->sleepq.lock);

	return 0;
}