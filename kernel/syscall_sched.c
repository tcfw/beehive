#include <kernel/list.h>
#include <kernel/thread.h>
#include <kernel/sched.h>
#include <kernel/syscall.h>
#include <kernel/uaccess.h>
#include <errno.h>

DEFINE_SYSCALL2(syscall_sched_getaffinity, SYSCALL_SCHED_GETAFFINITY, pid_t, pid, uint64_t *,affinity)
	int access = access_ok(ACCESS_TYPE_WRITE, affinity, sizeof(uint64_t));
	if (access < 0)
		return access;

	thread_t *curthread = get_thread_by_pid(pid);
	if (curthread == 0)
		return -ERRNOPROC;

	copy_to_user(&curthread->affinity, affinity, sizeof(uint64_t));

	return 0;
}

DEFINE_SYSCALL2(syscall_sched_setaffinity, SYSCALL_SCHED_SETAFFINITY, pid_t, pid, const uint64_t *,affinity)
	return -ERRNOSYS;
}