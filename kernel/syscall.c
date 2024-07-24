#include <kernel/stdint.h>
#include <errno.h>
#include <kernel/irq.h>
#include <kernel/syscall.h>
#include <kernel/tty.h>
#include <kernel/uaccess.h>
#include <kernel/clock.h>
#include <kernel/cls.h>
#include <syscall_num.h>

struct syscall_handler_t syscall_handlers[SYSCALL_MAX];

// syscall entry router
uint64_t ksyscall_entry(uint64_t type, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4)
{
	struct clocksource_t *clk = clock_first(CS_GLOBAL);
	thread_t *cthread = current;

	uint64_t clkval = clk->val(clk);
	if ((cthread->flags & THREAD_KTHREAD) == 0)
		cthread->timing.total_user += clkval - cthread->timing.last_user;
	cthread->timing.last_system = clkval;

	uint64_t ret = -ERRNOSYS;
	struct syscall_handler_t *handler = &syscall_handlers[(int)type];

	// terminal_logf("SYSCALL: S=%d TID=0x%X:0x%X\r\na0=0x%X a1=0x%X\r\na2=0x%X a3=0x%X a4=0x%X", type, cthread->process->pid, cthread->tid, arg0, arg1, arg2, arg3, arg4);

	if (handler->handler)
		switch (handler->argc)
		{
		case 1:
			ret = handler->handler(cthread, arg0);
			break;
		case 2:
			ret = handler->handler(cthread, arg0, arg1);
			break;
		case 3:
			ret = handler->handler(cthread, arg0, arg1, arg2);
			break;
		case 4:
			ret = handler->handler(cthread, arg0, arg1, arg2, arg3);
			break;
		case 5:
			ret = handler->handler(cthread, arg0, arg1, arg2, arg3, arg4);
			break;
		default:
			ret = handler->handler(cthread);
			break;
		}
	else
		terminal_logf("unhandled SYSCALL 0x%X TID=0x%X:0x%X", type, cthread->process->pid, cthread->tid);

	clkval = clk->val(clk);
	cthread->timing.total_system += clkval - cthread->timing.last_system;
	if ((cthread->flags & THREAD_KTHREAD) == 0)
		cthread->timing.last_user = clkval;

	if (cthread->state != THREAD_RUNNING)
		schedule();

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
