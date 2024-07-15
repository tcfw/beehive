#include <kernel/devices.h>
#include <kernel/devicetree.h>
#include <kernel/irq.h>
#include <kernel/list.h>
#include <kernel/mm.h>
#include <kernel/panic.h>
#include <kernel/regions.h>
#include <kernel/strings.h>
#include <kernel/sync.h>
#include <kernel/tty.h>
#include <kernel/vm.h>

LIST_HEAD(devices);
spinlock_t devices_lock;

struct list_head *get_devices_head()
{
	return &devices;
}

static char *virtio_subsystem_compatibility(void *dev_bar, size_t bar_size, char *compatibility)
{
	// Add virtio helpers to compatibility
	int32_t *bar = (int32_t *)(DEVICE_REGION + (uintptr_t)dev_bar);
	uintptr_t barpg = (uintptr_t)bar;
	if (barpg % PAGE_SIZE != 0)
		barpg -= barpg % PAGE_SIZE;
	uintptr_t barppg = (uintptr_t)dev_bar;
	if (barppg % PAGE_SIZE != 0)
		barppg -= barppg % PAGE_SIZE;

	size_t devpgsize = bar_size;
	if (devpgsize < PAGE_SIZE)
		devpgsize = PAGE_SIZE;

	if (devpgsize % PAGE_SIZE != 0)
		devpgsize += PAGE_SIZE - ((uintptr_t)devpgsize % PAGE_SIZE);
	devpgsize--;

	if (vm_map_region(vm_get_kernel(), barppg, barpg, devpgsize, MEMORY_TYPE_KERNEL | MEMORY_TYPE_DEVICE) < 0)
		panic("failed to map device region");

	if (*bar == 0x74726976)
	{
		char *virtio_subsys_type;
		switch (*(bar + 2)) // subsystem
		{
		case 1:
			virtio_subsys_type = "network-card";
			break;
		case 2:
			virtio_subsys_type = "block-device";
			break;
		case 3:
			virtio_subsys_type = "console";
			break;
		case 4:
			virtio_subsys_type = "entropy-source";
			break;
		case 5:
			virtio_subsys_type = "memory-ballooning";
			break;
		case 6:
			virtio_subsys_type = "iomemory";
			break;
		case 7:
			virtio_subsys_type = "rpmsg";
			break;
		case 8:
			virtio_subsys_type = "scsi-host";
			break;
		case 9:
			virtio_subsys_type = "9p-transport";
			break;
		case 10:
			virtio_subsys_type = "mac80211-wlan";
			break;
		case 11:
			virtio_subsys_type = "rproc-serial";
			break;
		case 12:
			virtio_subsys_type = "virtio-caif";
			break;
		case 13:
			virtio_subsys_type = "memory-balloon";
			break;
		case 16:
			virtio_subsys_type = "gpu-device";
			break;
		case 17:
			virtio_subsys_type = "timer/clock-device";
			break;
		case 18:
			virtio_subsys_type = "input-device";
			break;
		case 19:
			virtio_subsys_type = "socket-device";
			break;
		case 20:
			virtio_subsys_type = "crypto-device";
			break;
		case 21:
			virtio_subsys_type = "signal-distribution-module";
			break;
		case 22:
			virtio_subsys_type = "pstore-device";
			break;
		case 23:
			virtio_subsys_type = "iommu-device";
			break;
		case 24:
			virtio_subsys_type = "memory-device";
			break;
		case 0:
		default:
			virtio_subsys_type = "invalid";
			break;
		}

		uint32_t compatstrlen = strlen(compatibility);
		uint32_t sssl = strlen(virtio_subsys_type);
		size_t ss = compatstrlen + 1 + sssl + 1;

		char *newcompat = kmalloc(ss);
		memset(newcompat, 0, ss);
		strncpy(newcompat, compatibility, compatstrlen);
		strncpy(newcompat + compatstrlen, ",", 1);
		strncpy(newcompat + compatstrlen + 1, virtio_subsys_type, sssl);
		*(newcompat + ss + 1) = 0;

		compatibility = newcompat;
	}

	if (vm_unmap_region(vm_get_kernel(), (uintptr_t)barpg, devpgsize) < 0)
		terminal_log("failed to unmap device region");

	return compatibility;
}

void discover_devices()
{
	spinlock_acquire(&devices_lock);
	void *node = devicetree_get_next_node(devicetree_get_root_node());
	device_node_t *device = 0;
	char *name, *device_type;

	uint32_t discovered = 0;

	while (node != 0)
	{
		name = devicetree_get_node_name(node);
		if (strchr(name, '@') == 0)
			goto next;
		device_type = devicetree_get_property(node, "device_type");
		if (device_type != 0 && strcmp(device_type, "memory") == 0)
			goto next;

		if (devicetree_get_property(node, "interrupt-controller") != 0)
			goto next;

		device = kmalloc(sizeof(*device));
		device->id = discovered;
		device->name = name;
		device->device_type = device_type;
		device->compatibility = devicetree_get_property(node, "compatible");
		device->node = node;
		device->bar = (void *)devicetree_get_bar(node);
		device->bar_size = devicetree_get_bar_size(node);
		arch_get_device_interrupts(device, node);

		if (device->compatibility != 0)
			if (strcmp("virtio,mmio", device->compatibility) == 0)
			{
				// Add virtio helpers to compatibility
				device->compatibility = virtio_subsystem_compatibility(device->bar, device->bar_size, device->compatibility);
				uint64_t xrq = device->interrupt_set.interrupts[0].xrq;
				if (xrq != 0)
					enable_xrq_n(xrq);
			}

		if (devicetree_get_property(node, "dma-coherent") != 0)
			device->is_dma_coherent = 1;

		if (arch_should_handle_device(device))
		{
			arch_setup_device(device);
			goto next;
		}

		list_add_tail(&device->list, &devices);
		discovered++;
	next:
		node = devicetree_get_next_node(node);
	}

	terminal_logf("Discovered %d devices via devicetree", discovered);

	spinlock_release(&devices_lock);
}