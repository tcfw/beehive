#include <devices/pl031.h>
#include <kernel/clock.h>
#include <kernel/stdint.h>

uint64_t pl031_read_value(struct clocksource_t *cs)
{
	struct pl031_data *pd = (struct pl031_data *)cs->data;
	return *(uint32_t *)pd->bar;
}