#include "unistd.h"
#include <errno.h>
#include <kernel/irq.h>
#include <kernel/syscall.h>
#include <kernel/tty.h>

/*

// Impl syscall

int syscall1(uint32_t arg1)
{
	uint32_t ret;
	__asm__ volatile("mov x0, %0" ::"r"(arg1));
	__asm__ volatile("mov x1, #0");
	__asm__ volatile("mov x2, #0");
	__asm__ volatile("mov x3, #0");
	__asm__ volatile("svc #1");
	__asm__ volatile("mov %0, x0"
					 : "=r"(ret));

	return ret;
}


// Test syscall
int syscall_test(pid_t _, uint64_t arg0)
{
	char buf[50];
	ksprintf(&buf[0], "got syscall 1 => arg0: %x\n", arg0);
	terminal_writestring(buf);
	return 3;
}

*/

#define SYSCALL_MAX 60

struct syscall_handler_t syscall_handers[SYSCALL_MAX];

// syscall entry router
int ksyscall_entry(uint64_t type, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3)
{
	disable_xrq();

	int ret = -ERRNOSYS;
	struct syscall_handler_t *handler = &syscall_handers[(int)type];

	if (handler->handler)
	{
		switch (handler->argc)
		{
		case 1:
			ret = handler->handler(0, arg0);
			break;
		case 2:
			ret = handler->handler(0, arg0, arg1);
			break;
		case 3:
			ret = handler->handler(0, arg0, arg1, arg2);
			break;
		case 4:
			ret = handler->handler(0, arg0, arg1, arg2, arg3);
			break;
		default:
			ret = handler->handler(0);
			break;
		}
	}

	enable_xrq();

	return ret;
}

// Register a syscall to a handler
void register_syscall_handler(unsigned int n, syscall_handler_cb handler, uint8_t argc)
{
	if (n > SYSCALL_MAX)
		return;

	syscall_handers[n].argc = argc;
	syscall_handers[n].handler = handler;
}

void syscall_init()
{
	// syscall_handers[1].handler = &syscall_test;
	// syscall_handers[1].argc = 1;

	terminal_log("Loaded syscalls\n");
}