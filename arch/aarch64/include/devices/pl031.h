#ifndef _DEVICES_PL031_H
#define _DEVICES_PL031_H

#include <kernel/devices.h>
#include <kernel/stdint.h>
#include <kernel/clock.h>

struct pl031_data
{
	uintptr_t bar;
};

void arch_setup_pl031(device_node_t *);

uint64_t pl031_read_value(struct clocksource_t *cs);

#endif