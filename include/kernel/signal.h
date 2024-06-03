#ifndef _KERNEL_SIGNALS_H
#define _KERNEL_SIGNALS_H

#include <kernel/stdint.h>

#define SIG_SEGV (9)
#define SIG_MAX 22

#define SS_ONSTACK (1)
#define SS_DISABLE (2)

#define MINSIGSTKSIZ (4096)

typedef enum
{
	SIG_DEFAULT,
	SIG_IGNORE,
	SIG_HANDLER,
} SIGACTION;

typedef void (*sigaction_handler)(int);

typedef struct stack_t
{
	uintptr_t sp;
	uint32_t flags;
	uintptr_t size;
} stack_t;

typedef struct thread_sigaction_t
{
	SIGACTION action;
	sigaction_handler user_handler;
} thread_sigaction_t;

typedef struct thread_sigactions_t
{
	thread_sigaction_t sigaction[SIG_MAX];

	stack_t sig_stack;
} thread_sigactions_t;

#include <kernel/thread.h>

void send_signal(thread_t *thread, int signal);

#endif