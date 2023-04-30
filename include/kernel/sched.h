#ifndef _KERNEL_SCHED_H
#define _KERNEL_SCHED_H

#include <kernel/thread.h>

struct thread_sched_ctx_t
{
	// wait_cond
};

struct sched_queue_entry_t
{
	struct thread_t *current;
	struct sched_queue_entry_t *next;
};

#endif