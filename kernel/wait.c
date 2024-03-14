#include <kernel/cls.h>
#include <kernel/list.h>
#include <kernel/mm.h>
#include <kernel/sched.h>
#include <kernel/syscall.h>
#include <kernel/thread.h>
#include <kernel/wait.h>

void try_wake_waitqueue(waitqueue_head_t *wq)
{
	int state = spinlock_acquire_irq(&wq->lock);

	if (!list_is_empty(&wq->head))
	{
		cls_t *cls = get_cls();

		waitqueue_entry_t *current;
		waitqueue_entry_t *tmp;
		list_head_for_each_safe(current, tmp, &wq->head)
		{
			if (current->func(current, current->data) > 0)
			{
				wake_thread(current->thread);
				current->thread->sched_class->enqueue_thread(&cls->rq, current->thread);
				list_del(current);
				kfree(current);
			}
		}
	}

	spinlock_release_irq(state, &wq->lock);
}

int wq_can_wake_thread(waitqueue_entry_t *wq_entry, void *_)
{
	return can_wake_thread(wq_entry->thread);
}

DEFINE_SYSCALL2(syscall_nanosleep, SYSCALL_NANOSLEEP, void *, ts, void *, remaining)
	cls_t *cls = get_cls();
	spinlock_acquire(&cls->sleepq.lock);

	sleep_thread(thread, (timespec_t *)ts, (timespec_t *)remaining);

	waitqueue_entry_t *wqe = (waitqueue_entry_t *)kmalloc(sizeof(waitqueue_entry_t));
	wqe->thread = thread;
	wqe->func = wq_can_wake_thread;

	list_add_tail(wqe, &cls->sleepq.head);

	spinlock_release(&cls->sleepq.lock);
}