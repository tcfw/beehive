#ifndef _KERNEL_FUTEX_H
#define _KERNEL_FUTEX_H

#include <kernel/sync.h>
#include <kernel/list.h>
#include <kernel/stdint.h>
#include <kernel/thread.h>
#include <kernel/vm.h>

#define FUTEX_HASH_SEED (0x656a536539680f0aULL)
#define FUTEX_HASHBUCKETS_SIZE (1024)

#define FUTEX_OP_WAKE (0)
#define FUTEX_OP_SLEEP (1)

typedef struct thread_t thread_t;
typedef struct vm_t vm_t;

typedef uint64_t futex_hb_key;

union futex_key {
	struct {
		vm_t *vm;
		uintptr_t addr;
	} private;
	struct {
		uintptr_t addr;
		uintptr_t _tmp;
	} shared;
	struct {
		uintptr_t ptr;
		uintptr_t word;
	} both;
};

typedef struct futex_queue_t {
	struct list_head list;

	union futex_key key;
	int wanted_value;
	thread_t *thread;
} futex_queue_t;

typedef struct futex_hb_t {
	spinlock_t lock;
	struct list_head chain;
} futex_hb_t;

void futex_init(void);

futex_hb_key futex_hash_key(union futex_key *key);

futex_hb_t *futex_hb(union futex_key *key);

static inline int futex_keys_match(union futex_key *a, union futex_key *b) {
	return (a && b 
		&& a->both.ptr == b->both.ptr
		&& a->both.word == b->both.word);
}

int futex_get_key(void *uaddr, union futex_key* key);

int futex_do_wake(void *uaddr, uint32_t n_wake, uint32_t val);

int futex_do_sleep(void *uaddr, uint32_t val, timespec_t* timeout);

#endif