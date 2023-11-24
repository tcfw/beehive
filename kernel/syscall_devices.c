#include <kernel/devices.h>
#include <kernel/syscall.h>
#include <kernel/uaccess.h>
#include <kernel/thread.h>

int syscall_dev_count(thread_t *thread)
{
	int count = 0;

	device_node_t *node;
	list_head_for_each(node, get_devices_head())
	{
		count++;
	}

	return count;
}

SYSCALL(SYSCALL_DEV_COUNT, syscall_dev_count, 0)