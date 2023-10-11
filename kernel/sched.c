#include <kernel/cls.h>
#include <kernel/list.h>
#include <kernel/mm.h>
#include <kernel/sched.h>
#include <kernel/sync.h>
#include <kernel/thread.h>

static LIST_HEAD(pending);
static spinlock_t pending_lock;

void sched_init(void)
{
	spinlock_init(&pending_lock);
	INIT_LIST_HEAD(&pending);
}

void sched_local_init(void)
{
	cls_t *cls = get_cls();
	spinlock_init(&cls->thread_queue.lock);
}

thread_t *sched_get_pending(uint64_t affinity)
{
	spinlock_acquire(&pending_lock);

	if (list_empty(pending))
	{
		spinlock_release(&pending_lock);
		return 0;
	}

	struct thread_list_entry_t *current;
	list_head_foreach(current, &pending)
	{
		if ((current->thread->affinity & affinity) != 0)
		{
			thread_t *thread = current->thread;
			list_del((struct list_head *)current);
			page_free(current);
			spinlock_release(&pending_lock);
			return thread;
		}
	}

	spinlock_release(&pending_lock);
	return 0;
}

void sched_append_pending(thread_t *thread)
{
	spinlock_acquire(&pending_lock);

	struct thread_list_entry_t *entry = (struct thread_list_entry_t *)page_alloc_s(sizeof(struct thread_list_entry_t));
	entry->thread = thread;

	list_add_tail((struct list_head *)entry, &pending);

	spinlock_release(&pending_lock);
}

uint64_t sched_affinity(uint64_t cpu_id)
{
	return 1 << cpu_id;
}