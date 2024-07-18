#include "errno.h"
#include <kernel/devices.h>
#include <kernel/devicetree.h>
#include <kernel/irq.h>
#include <kernel/mm.h>
#include <kernel/strings.h>
#include <kernel/syscall.h>
#include <kernel/thread.h>
#include <kernel/tty.h>
#include <kernel/uaccess.h>
#include <kernel/umm.h>

DEFINE_SYSCALL0(syscall_dev_count, SYSCALL_DEV_COUNT)
{
	int count = 0;

	device_node_t *node;
	list_head_for_each(node, get_devices_head())
	{
		count++;
	}

	return count;
}

DEFINE_SYSCALL2(syscall_dev_info, SYSCALL_DEV_INFO, const uint32_t, id, struct dev_info *, uinfo)
{
	int ok = access_ok(ACCESS_TYPE_WRITE, uinfo, sizeof(*uinfo));
	if (ok < 0)
		return ok;

	device_node_t *node;
	list_head_for_each(node, get_devices_head())
	{
		if (node->id == id)
		{
			struct dev_info info = {
				.id = node->id,
				.phy_bar = (uint64_t)node->bar,
				.phy_bar_size = node->bar_size,
				// .interrupts = node->interrupt_set,
			};

			strncpy(&info.name, node->name, sizeof(info.name));

			if (node->compatibility != 0)
				strncpy(&info.type, node->compatibility, sizeof(info.type));
			else if (node->device_type != 0)
				strncpy(&info.type, node->device_type, sizeof(info.type));

			copy_to_user(&info, uinfo, sizeof(info));

			return 0;
		}
	}

	return -ERRNOENT;
}

DEFINE_SYSCALL5(syscall_dev_prop, SYSCALL_DEV_PROP, const uint32_t, id, const char *, prop, size_t, prop_len, void *, value, size_t, value_len)
{
	int ok = access_ok(ACCESS_TYPE_READ, prop, prop_len);
	if (ok < 0)
		return ok;

	ok = access_ok(ACCESS_TYPE_WRITE, value, value_len);
	if (ok < 0)
		return ok;

	if (prop_len > 50 || value_len > 2048)
		return -ERRFAULT;

	device_node_t *node;
	list_head_for_each(node, get_devices_head())
	{
		if (node->id == id)
		{
			char *propRef = kmalloc(prop_len + 1); // forced null term
			copy_from_user(prop, propRef, prop_len);
			char *propVal = devicetree_get_property(node->node, propRef);
			uint32_t propValLen = devicetree_get_property_len(node->node, propRef);
			kfree(propRef);

			if (propVal == 0)
				return -ERRINVAL;

			if (propValLen > value_len)
				return -ERRSIZE;

			copy_to_user((void *)propVal, value, propValLen);

			return propValLen;
		}
	}

	return -ERRNOENT;
}

DEFINE_SYSCALL1(syscall_dev_phy_addr, SYSCALL_DEV_PHY_ADDR, uintptr_t, vaddr)
{
	vm_mapping *map = has_mapping(thread, vaddr, 1);
	if (map == 0)
		return -ERRINVAL;

	if ((map->flags & VM_MAP_FLAG_DEVICE) == 0 || map->page == 0)
		return -ERRACCESS;

	return map->phy_addr;
}

DEFINE_SYSCALL1(syscall_dev_irq_ack, SYSCALL_DEV_IRQ_ACK, uint64_t, irq)
{
	// TODO(tcfw) allow acking only owned
	ack_xrq(irq);
	terminal_logf("acking 0x%X", irq);

	return 0;
}