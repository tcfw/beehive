#include <kernel/cls.h>
#include <kernel/list.h>
#include <kernel/mm.h>
#include <kernel/wait.h>

void try_wake_waitqueue(waitqueue_head_t *wq)
{
	int state = spinlock_acquire_irq(&wq->lock);

	cls_t *cls = get_cls();

	waitqueue_entry_t *current;

	if (!list_is_empty(&wq->head))
	{
		list_head_foreach(current, &wq->head)
		{
			if (current->func(wq, current->data) > 0)
			{
				current->thread->sched_class->enqueue_thread(&cls->rq, current->thread);
				list_del(current);
				kfree(current);
			}
		}
	}

	spinlock_release_irq(state, &wq->lock);
}