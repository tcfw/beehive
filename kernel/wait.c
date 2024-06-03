#include <kernel/cls.h>
#include <kernel/mm.h>
#include <kernel/sync.h>
#include <kernel/wait.h>

void try_wake_waitqueue(waitqueue_head_t *wq)
{
	cls_t *cls = get_cls();

	if (!list_is_empty(&wq->head))
	{
		waitqueue_entry_t *this;
		waitqueue_entry_t *tmp;
		list_head_for_each_safe(this, tmp, &wq->head)
		{
			if (this->thread->process->state != RUNNING)
				continue;

			if (this->func(this) > 0)
			{
				wake_thread(this->thread);
				list_del(this);
				kfree(this);
			}
		}
	}
}

int wq_can_wake_thread(waitqueue_entry_t *wq_entry)
{
	timespec_t ts;
	timespec_t d;
	struct clocksource_t *cs;

	if (wq_entry->timeout)
	{
		cs = clock_first(CS_GLOBAL);
		timespec_from_cs(cs, &ts);
		timespec_diff(&ts, wq_entry->timeout, &d);
		if (d.seconds >= 0 && d.nanoseconds >= 0)
			return 1;
	}

	if (wq_entry->thread->state == THREAD_DEAD)
		return -2;

	return can_wake_thread(wq_entry->thread);
}
