#ifndef _KERNEL_THREAD_H
#define _KERNEL_THREAD_H

#include "unistd.h"
#include <kernel/context.h>
#include <kernel/limits.h>
#include <kernel/sched.h>
#include <kernel/signal.h>
#include <kernel/vm.h>

typedef struct thread_t
{
	pid_t pid;
	pid_t ppid;

	char cmd[CMD_MAX];
	unsigned int argc;
	char argv[ARG_MAX];

	unsigned int uid;
	unsigned int euid;
	unsigned int gid;
	unsigned int egid;

	context_t ctx;
	vm_table *vm_table;
	struct thread_sigactions_t *sigactions;

	// wait cond
	//  shared mem maps
	//  queues
} thread_t;

// Init a thread
void init_thread(thread_t *thread);

// Init a thread context
void init_context(context_t *ctx);

#endif