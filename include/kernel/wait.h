#ifndef _KERNEL_WAITQUEUE_H
#define _KERNEL_WAITQUEUE_H

#include <kernel/list.h>
#include <kernel/thread.h>
#include <kernel/sync.h>

typedef struct waitqueue_head_t
{
	struct list_head head;
	spinlock_t lock;
} waitqueue_head_t;

typedef struct waitqueue_entry_t waitqueue_entry_t;

typedef int (*waitqueue_func_t)(waitqueue_entry_t *wq_entry, void *data);

typedef struct waitqueue_entry_t
{
	struct list_head list;
	thread_t *thread;
	waitqueue_func_t func;
	void *data;
} waitqueue_entry_t;

static inline void INIT_WAITQUEUE(waitqueue_head_t *wq)
{
	INIT_LIST_HEAD(&wq->head);
	spinlock_init(&wq->lock);
}

void try_wake_waitqueue(waitqueue_head_t *wq);

#endif