#ifndef _KERNEL_SYSCALL_H
#define _KERNEL_SYSCALL_H

#include "stdint.h"
#include <kernel/thread.h>
#include <kernel/modules.h>

typedef int (*syscall_handler_cb)(pid_t pid, ...);

struct syscall_handler_t
{
	syscall_handler_cb handler;
	uint8_t argc;
};

// Register a syscall handler
void register_syscall_handler(unsigned int n, syscall_handler_cb handler, uint8_t argc);

// Handle a syscall entry
int ksyscall_entry(uint64_t type, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3);

// Initiate builtin syscalls
void syscall_init();

#define SYSCALL(n, handler, argc)                   \
	static void syscall_reg_##handler(void)         \
	{                                               \
		register_syscall_handler(n, handler, argc); \
	}                                               \
	MOD_INIT(syscall_reg_##handler);

#endif