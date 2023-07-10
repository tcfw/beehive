#ifndef _KERNEL_CLS_H
#define _KERNEL_CLS_H

#include <kernel/thread.h>
#include "stdint.h"

typedef struct cls_t
{
	uint64_t id;
	thread_t *currentThread;
} cls_t;

cls_t *get_cls(void);
void init_cls(uint8_t n);

#endif