#ifndef _KERNEL_SYSCALL_H
#define _KERNEL_SYSCALL_H

#include "unistd.h"
#include "stdint.h"
#include <kernel/thread.h>

typedef int (*syscall_handler_cb)(pid_t pid, ...);

struct syscall_handler_t
{
	syscall_handler_cb handler;
	uint8_t argc;
};

void register_syscall_handler(unsigned int n, syscall_handler_cb handler, uint8_t argc);

int ksyscall_entry(uint64_t type, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3);

void syscall_init();

#endif