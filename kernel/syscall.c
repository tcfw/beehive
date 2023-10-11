#include "stdint.h"
#include <errno.h>
#include <kernel/irq.h>
#include <kernel/syscall.h>
#include <kernel/tty.h>
#include <kernel/uaccess.h>
#include <kernel/clock.h>
#include <kernel/cls.h>

#define SYSCALL_MAX 140

struct syscall_handler_t syscall_handlers[SYSCALL_MAX];

// syscall entry router
int ksyscall_entry(uint64_t type, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3)
{
	disable_irq();

	struct clocksource_t *clk = clock_first(CS_GLOBAL);
	thread_t *cthread = get_cls()->currentThread;

	uint64_t clkval = clk->val(clk);
	cthread->timing.total_user += clkval - cthread->timing.last_user;
	cthread->timing.last_system = clkval;

	int ret = -ERRNOSYS;
	struct syscall_handler_t *handler = &syscall_handlers[(int)type];

	if (handler->handler)
	{
		switch (handler->argc)
		{
		case 1:
			ret = handler->handler(cthread->pid, arg0);
			break;
		case 2:
			ret = handler->handler(cthread->pid, arg0, arg1);
			break;
		case 3:
			ret = handler->handler(cthread->pid, arg0, arg1, arg2);
			break;
		case 4:
			ret = handler->handler(cthread->pid, arg0, arg1, arg2, arg3);
			break;
		default:
			ret = handler->handler(cthread->pid);
			break;
		}
	}

	clkval = clk->val(clk);
	cthread->timing.total_system += clkval - cthread->timing.last_system;
	cthread->timing.last_user = clkval;

	enable_irq();

	return ret;
}

// Register a syscall to a handler
void register_syscall_handler(unsigned int n, syscall_handler_cb handler, uint8_t argc)
{
	if (n > SYSCALL_MAX)
		return;

	syscall_handlers[n].argc = argc;
	syscall_handlers[n].handler = handler;
}

void syscall_init()
{
	// syscall_handlers[131].handler = &syscall_test;
	// syscall_handlers[131].argc = 2;

	terminal_log("Loaded syscalls");
}