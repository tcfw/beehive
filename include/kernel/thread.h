#ifndef _KERNEL_THREAD_H
#define _KERNEL_THREAD_H

#include "unistd.h"
#include <kernel/context.h>
#include <kernel/limits.h>
#include <kernel/signal.h>
#include <kernel/vm.h>

typedef struct thread_timing_t
{
	uint64_t total_system;
	uint64_t total_user;
	uint64_t total_wait;

	uint64_t last_system;
	uint64_t last_user;
	uint64_t last_wait;
} thread_timing_t;

typedef struct thread_t
{
	pid_t pid;
	struct thread_t *parent;

	char cmd[CMD_MAX];
	char argv[ARG_MAX];

	unsigned int uid;
	unsigned int euid;
	unsigned int gid;
	unsigned int egid;

	context_t ctx;
	uint64_t flags;
	uint64_t affinity;
	vm_table *vm_table;
	thread_timing_t timing;

	thread_sigactions_t *sigactions;

	// wait cond
	//  shared mem maps
	//  queues
} thread_t;

// Init a thread
void init_thread(thread_t *thread);

// Init a thread context
void init_context(context_t *ctx);

void arch_thread_prep_switch(thread_t *thread);

#endif