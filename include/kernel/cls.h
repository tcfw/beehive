#ifndef _KERNEL_CLS_H
#define _KERNEL_CLS_H

#include <kernel/thread.h>
#include <kernel/sched.h>
#include "stdint.h"

enum exception_operation
{
	EXCEPTION_UNKNOWN = 0,
	EXCEPTION_USER_COPY_TO = 1,
	EXCEPTION_USER_COPY_FROM = 2,
};

// Core local storage object
// Holds the stateful store for each processor
typedef struct cls_t
{
	uint64_t id;
	thread_t *currentThread;
	schedule_queue thread_queue;

	// cause for exception handler
	enum exception_operation cfe;
	void (*cfe_handle)(void);
} cls_t;

// Get the core local storage object for the current core
cls_t *get_cls(void);

// Init the core local storage for all cores given the required
// number of cores
void init_cls(uint8_t n);

// Set the current working thread
void set_current_thread(thread_t *thread);

#endif