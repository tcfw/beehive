#include <kernel/devices.h>
#include <kernel/syscall.h>
#include <kernel/uaccess.h>
#include <kernel/thread.h>

DEFINE_SYSCALL0(syscall_dev_count, SYSCALL_DEV_COUNT)
	int count = 0;

	device_node_t *node;
	list_head_for_each(node, get_devices_head())
	{
		count++;
	}

	return count;
}