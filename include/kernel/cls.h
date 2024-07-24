#ifndef _KERNEL_CLS_H
#define _KERNEL_CLS_H

#include <kernel/sched.h>
#include <kernel/stdint.h>
#include <kernel/thread.h>
#include <kernel/wait.h>

#define is_current_cls(cls) cls == get_cls()

enum exception_operation
{
	EXCEPTION_UNKNOWN = 0,
	EXCEPTION_USER_COPY_TO = 1,
	EXCEPTION_USER_COPY_FROM = 2,
};

enum interrupt_cause
{
	INTERRUPT_CAUSE_UNKNOWN = 0,
	INTERRUPT_CAUSE_SWI = 1,
	INTERRUPT_CAUSE_PENDING_IRQ = 2,
	INTERRUPT_CAUSE_IRQ = 3,
};

// Core local storage object
// Holds the stateful store for each processor
typedef struct cls_t
{
	uint32_t id;
	enum interrupt_cause irq_cause;
	uint64_t pending_irq;

	// schedule runqueue
	sched_rq_t rq;

	// sleep queue
	waitqueue_head_t sleepq;

	// cause for exception handler
	enum exception_operation cfe;
	void (*cfe_handle)(void);
} cls_t;

#define current get_current_thread()

// Get the core local storage object for the current core
cls_t *get_cls(void);

// Get the core local storage object for a specific core
cls_t *get_core_cls(uint8_t n);

// Init the core local storage for all cores
void init_cls();

// Set the current working thread
void set_current_thread(thread_t *thread);

static inline thread_t *get_current_thread()
{
	return get_cls()->rq.current_thread;
}

static inline enum interrupt_cause get_cls_irq_cause()
{
	return get_cls()->irq_cause;
}

static inline void set_cls_irq_cause(enum interrupt_cause cause)
{
	get_cls()->irq_cause = cause;
}

static inline void clear_cls_irq_cause()
{
	get_cls()->irq_cause = INTERRUPT_CAUSE_UNKNOWN;
}

#endif