#include <kernel/signal.h>
#include <kernel/thread.h>
#include <kernel/tty.h>

static void default_signal_handler(thread_t *thread, int signal)
{
	switch (signal)
	{
	case SIG_SEGV:
		terminal_logf("thread died TID=0x%X:0x%X PC=0x%X", thread->process->pid, thread->tid, thread->ctx.pc);
		set_thread_state(thread, THREAD_DEAD);
		break;
	}

	return;
}

void send_signal(thread_t *thread, int signal)
{
	if (signal >= SIG_MAX)
		return;

	thread_sigaction_t *action = &thread->sigactions.sigaction[signal];

	switch (action->action)
	{
	case SIG_IGNORE:
		// noop
		break;
	case SIG_DEFAULT:
	default:
		default_signal_handler(thread, signal);
		break;
	case SIG_HANDLER:
		break;
	}

	return;
}