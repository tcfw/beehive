#ifndef _KERNEL_DEVICES_H
#define _KERNEL_DEVICES_H

#include <kernel/unistd.h>
#include <kernel/stdint.h>
#include <kernel/list.h>

typedef struct device_node_t
{
	struct list_head list;
	char *name;
	char *device_type;
	char *compatibility;
	uint64_t bar_size;
	void *bar;
	struct device_node_property_t *properties;
	unsigned int is_dma_coherent : 1;
} device_node_t;

typedef struct device_node_property_t
{
	struct device_node_property_t *next;
	char *key;
	size_t data_len;
	void *data;
} device_node_property_t;

int discover_devices(uintptr_t ddr);

struct list_head *get_devices_head();

#endif