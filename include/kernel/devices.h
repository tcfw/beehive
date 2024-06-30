#ifndef _KERNEL_DEVICES_H
#define _KERNEL_DEVICES_H

#include <kernel/unistd.h>
#include <kernel/stdint.h>
#include <kernel/list.h>
#include <kernel/device_interrupt.h>

typedef struct device_interrupt_set_t
{
	struct list_head list;
	device_interrupt_t interrupts[5];
} device_interrupt_set_t;

typedef struct device_node_t
{
	struct list_head list;

	uint32_t id;
	char *name;

	char *device_type;
	char *compatibility;

	uint64_t bar_size;
	void *bar;

	void *node;

	struct device_node_property_t *properties;

	unsigned int is_dma_coherent : 1;

	device_interrupt_set_t interrupt_set;
} device_node_t;

typedef struct device_node_property_t
{
	struct device_node_property_t *next;
	char *key;
	size_t data_len;
	void *data;
} device_node_property_t;

struct dev_info
{
	uint32_t id;
	char name[64];
	char type[64];
	uint64_t phy_bar;
	uint64_t phy_bar_size;
	uint64_t interrupts[5];
};

void discover_devices();

struct list_head *get_devices_head();

#endif