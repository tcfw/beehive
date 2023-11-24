#include <kernel/devices.h>
#include <kernel/list.h>
#include <kernel/sync.h>

LIST_HEAD(devices);
spinlock_t devices_lock;

struct list_head *get_devices_head()
{
	return &devices;
}