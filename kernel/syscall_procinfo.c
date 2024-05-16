#include <kernel/syscall.h>
#include <kernel/thread.h>
#include <kernel/uaccess.h>
#include <kernel/cls.h>
#include <errno.h>

DEFINE_SYSCALL1(syscall_get_cpu, SYSCALL_GET_CPU, uint32_t *, cpu)
{
	cls_t *cls = get_cls();

	int access = access_ok(ACCESS_TYPE_WRITE, cpu, sizeof(uint32_t));
	if (access < 0)
		return access;

	copy_to_user(&cls->id, &cpu, sizeof(uint32_t));

	return 0;
}

DEFINE_SYSCALL0(syscall_get_pid, SYSCALL_GET_PID)
{
	return thread->process->pid;
}

DEFINE_SYSCALL0(syscall_get_tid, SYSCALL_GET_TID)
{
	return thread->tid;
}

DEFINE_SYSCALL0(syscall_get_ppid, SYSCALL_GET_PPID)
{
	if (thread->process->parent != 0)
		return thread->process->parent->pid;

	return 0;
}