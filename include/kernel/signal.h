#ifndef _KERNEL_SIGNALS_H
#define _KERNEL_SIGNALS_H

#define SIG_MAX 22

typedef enum
{
	SIG_DEFAULT,
	SIG_IGNORE,
	SIG_HANDLER,
} SIGACTION;

typedef void (*sigaction_handler)(int);

typedef struct thread_sigaction_t
{
	SIGACTION action;
	sigaction_handler user_handler;
} thread_sigaction_t;

typedef struct thread_sigactions_t
{
	thread_sigaction_t sigaction[SIG_MAX];
} thread_sigactions_t;

#endif