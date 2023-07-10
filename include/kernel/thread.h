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

	char cmd[CMD_MAX];
	unsigned int argc;
	char argv[ARG_MAX];

	unsigned int uid;
	unsigned int euid;
	unsigned int gid;
	unsigned int egid;

	struct context_t ctx;
	struct vm_table_t vm;
	struct thread_sigactions_t sigactions;

	// wait cond
	//  shared mem maps
	//  allocated pages
	//  queues
} thread_t;

void init_thread(thread_t *thread);
void init_context(context_t *ctx);

#endif