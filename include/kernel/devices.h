#ifndef _KERNEL_DEVICES_H
#define _KERNEL_DEVICES_H

#include "unistd.h"
#include "stdbool.h"

struct device_node_t
{
	char *name;
	char *device_type;
	char *compatibility;
	uint64_t bar_size;
	void *bar;
	struct device_node_property_t *properties;
	bool is_dma_coherent;
};

struct device_node_property_t
{
	char *key;
	size_t data_len;
	void *data;
	struct device_node_property_t *next;
};

#endif