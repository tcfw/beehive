#include <kernel/debug.h>
#include <kernel/thread.h>
#include <kernel/tty.h>

void user_debug_software_step_handler(thread_t *thread)
{
	terminal_logf("captured software step in thread %d:%d, pc=0x%X", thread->process->pid, thread->tid, thread->ctx.pc);
	thread_disable_single_step(&thread->ctx);
	disable_hw_debugger();
}