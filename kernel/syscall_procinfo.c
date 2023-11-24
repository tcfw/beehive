#include <kernel/syscall.h>
#include <kernel/thread.h>
#include <kernel/uaccess.h>
#include <kernel/cls.h>
#include "errno.h"

int syscall_get_cpu(thread_t *thread, uint32_t *cpu)
{
	cls_t *cls = get_cls();

	int access = access_ok(ACCESS_TYPE_WRITE, cpu, sizeof(uint32_t));
	if (access < 0)
		return access;

	copy_to_user(&cls->id, &cpu, sizeof(uint32_t));

	return 0;
}

int syscall_get_pid(thread_t *thread)
{
	return thread->pid;
}

int syscall_get_tid(thread_t *thread)
{
	return thread->tid;
}

int syscall_get_ppid(thread_t *thread)
{
	return thread->parent->pid;
}

SYSCALL(SYSCALL_GET_CPU, syscall_get_cpu, 1);
SYSCALL(SYSCALL_GET_PID, syscall_get_pid, 0);
SYSCALL(SYSCALL_GET_PPID, syscall_get_ppid, 0);
SYSCALL(SYSCALL_GET_TID, syscall_get_tid, 0);