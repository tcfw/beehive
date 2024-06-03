#include <kernel/devicetree.h>
#include <kernel/msgs.h>
#include <kernel/paging.h>
#include <kernel/syscall.h>
#include <kernel/sysinfo.h>
#include <kernel/thread.h>
#include <kernel/uaccess.h>

#define SYSINFO_OP_FIELD_NCPU (1)
#define SYSINFO_OP_FIELD_PAGE_SIZE (2)

DEFINE_SYSCALL1(syscall_kname, SYSCALL_KNAME, struct kname *, data)
{
	int access = access_ok(ACCESS_TYPE_WRITE, data, sizeof(struct kname));
	if (access < 0)
		return access;

	struct kname info =
		{
			.name = HELLO_HEADER,
			.version = SYSINFO_VERSION,
			.arch = SYSINFO_ARCH,
		};

	copy_to_user(&info, data, sizeof(info));

	return 0;
}

DEFINE_SYSCALL2(syscall_sysinfo, SYSCALL_SYSINFO, uint64_t, op, struct sysinfo *, data)
{
	switch (op)
	{
	case 0: // sysinfo
		goto sysinfo;
	case SYSINFO_OP_FIELD_NCPU: // ncpu
		return devicetree_count_dev_type("cpu");
	case SYSINFO_OP_FIELD_PAGE_SIZE: // page size
		return PAGE_SIZE;
	}

sysinfo:
	int access = access_ok(ACCESS_TYPE_WRITE, data, sizeof(*data));
	if (access < 0)
		return access;

	struct sysinfo info =
		{
			.ncpu = devicetree_count_dev_type("cpu"),
			.page_size = PAGE_SIZE,
		};

	copy_to_user(&info, data, sizeof(info));

	return 0;
}