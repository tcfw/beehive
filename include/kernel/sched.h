#ifndef _KERNEL_SCHED_H
#define _KERNEL_SCHED_H

#include <kernel/list.h>
#include <kernel/sync.h>
#include <kernel/thread.h>
#include "stdint.h"

#define MAX_PRIORITY (10)

typedef struct thread_list_entry_t
{
	struct list_head list;
	thread_t *thread;
} thread_list_entry_t;

typedef struct priority_queue_t
{
	thread_list_entry_t *active;
	thread_list_entry_t *complete;
} priority_queue_t;

typedef struct sched_mlq_t
{
	priority_queue_t queue[MAX_PRIORITY];
	spinlock_t lock;
	uint8_t active_priority;
} sched_mlq_t;

typedef sched_mlq_t schedule_queue;

void sched_init(void);

void sched_local_init(void);

thread_t *sched_get_pending(uint64_t affinity);

void sched_append_pending(thread_t *thread);

uint64_t sched_affinity(uint64_t cpu_id);

#endif