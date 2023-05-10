#include <kernel/sched.h>
#include <kernel/list.h>

struct thread_list_t
{
	struct list_head list;
	struct thread_t *thread;
};

struct list_head sched_threads;

struct sched_queue_entry_t *sched_head;
