#ifndef _KERNEL_FUTEX_H
#define _KERNEL_FUTEX_H

#include <kernel/sync.h>
#include <kernel/list.h>
#include <kernel/stdint.h>

union futex_key {
	struct {
		uintptr_t addr;
	} private;
	struct {
		uintptr_t addr;
	} shared;
	struct {
		uintptr_t ptr;
	} both;
};

typedef struct futex_queue_t {
	struct list_head list;

	union futex_key key;
} futex_queue_t;

typedef struct futex_hb_t {
	spinlock_t lock;
	struct list_head chain;
} futex_hb_t;


#endif