#ifndef _KERNEL_CLS_H
#define _KERNEL_CLS_H

#include <kernel/thread.h>
#include "stdint.h"

// Core local storage object
// Holds the stateful store for each processor
typedef struct cls_t
{
	uint64_t id;
	thread_t *currentThread;
} cls_t;

// Get the core local storage object for the current core
cls_t *get_cls(void);

// Init the core local storage for all cores given the required
// number of cores
void init_cls(uint8_t n);

#endif