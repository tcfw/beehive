#ifndef _KERNEL_SCHED_H
#define _KERNEL_SCHED_H

#include <kernel/list.h>
#include <kernel/sync.h>
#include <kernel/thread.h>
#include <kernel/skiplist.h>
#include "stdint.h"

#define SCHED_MIN_TICK_DURATION (0UL)

#define MAX_PRIO (20)
#define PRIO_INTERVAL_BASE (50000.0)

enum Sched_Classes
{
	SCHED_CLASS_LRF = 0,
	SCHED_CLASS_IDLE = 1,
};

typedef struct thread_t thread_t;
typedef struct sched_class_t sched_class_t;

typedef struct sched_rq_t
{
	thread_t *current_thread;

	skiplist_t lrf;
	thread_t *idle;

	spinlock_t lock;

	uint64_t last_tick;
} sched_rq_t;

typedef struct sched_class_t
{
	sched_class_t *next;

	enum Sched_Classes class;

	// Put a new thread in the queue
	void (*enqueue_thread)(sched_rq_t *rq, thread_t *thread);

	// Remove a thread from the queue
	void (*dequeue_thread)(sched_rq_t *rq, thread_t *thread);

	// Put a thread back in the queue
	void (*requeue_thread)(sched_rq_t *rq, thread_t *thread);

	// Get the next thread to run
	thread_t *(*next_thread)(sched_rq_t *rq);

	// Denote when this queue is about to be entered
	void (*tick)(sched_rq_t *rq);

} sched_class_t;

typedef struct sched_entity_t
{
	int64_t deadline;
	uint64_t last_deadline;
	uint64_t prio;
} sched_entity_t;

void sched_init(void);

void sched_local_init(void);

thread_t *sched_get_pending(uint64_t affinity);

void sched_append_pending(thread_t *thread);

uint64_t sched_affinity(uint64_t cpu_id);

sched_class_t *sched_get_class(enum Sched_Classes class);

void schedule(void);

void schedule_start(void);

#endif