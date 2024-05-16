#include <kernel/msgs.h>
#include <kernel/syscall.h>
#include <kernel/sysinfo.h>
#include <kernel/thread.h>
#include <kernel/uaccess.h>

DEFINE_SYSCALL1(syscall_kname, SYSCALL_KNAME, struct sysinfo *, buf)
{
	int access = access_ok(ACCESS_TYPE_WRITE, buf, sizeof(struct sysinfo));
	if (access < 0)
		return access;

	struct sysinfo info =
		{
			.name = HELLO_HEADER,
			.version = SYSINFO_VERSION,
			.arch = SYSINFO_ARCH,
		};

	copy_to_user(&info, buf, sizeof(struct sysinfo));

	return 0;
}