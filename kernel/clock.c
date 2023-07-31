#include <kernel/clock.h>
#include <kernel/list.h>
#include <kernel/panic.h>

struct clocksource_t *clockSources;

void RegisterClockSource(struct clocksource_t *cs)
{
	clockSources = list_head_append(struct clocksource_t *, clockSources, cs);
}

struct clocksource_t *clock_first(enum ClockSourceType type)
{
	struct clocksource_t *cs = clockSources;
	if (cs->type == type)
		return cs;

	list_head_foreach(cs, clockSources)
	{
		if (cs->type == type)
			return cs;
	}

	return 0;
}

void global_clock_init()
{
	struct clocksource_t *cs = clock_first(CS_GLOBAL);
	if (cs == 0)
	{
		panic("No global clock");
		return;
	}

	cs->disableIRQ(cs, 0);
	cs->enable(cs);
}
