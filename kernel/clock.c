#include <kernel/clock.h>
#include <kernel/list.h>
#include <kernel/panic.h>
#include <kernel/stdint.h>

static LIST_HEAD(clockSources);

void RegisterClockSource(struct clocksource_t *cs)
{
	list_add((struct list_head *)cs, &clockSources);
}

struct clocksource_t *clock_first(enum ClockSourceType type)
{
	struct clocksource_t *cs;

	list_head_for_each(cs, &clockSources)
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

	cs->disableIRQ(cs);
	cs->enable(cs);
}

void timespec_from_cs(struct clocksource_t *cs, timespec_t *ts)
{
	uint64_t cc = cs->val(cs);
	uint64_t freq = cs->getFreq(cs);

	uint64_t seconds = cc / freq;
	uint64_t clock_nanos = 1000000000 / freq;
	uint64_t nano = (cc - (seconds * freq)) * clock_nanos;
	ts->seconds = seconds;
	ts->nanoseconds = nano;
}

void timespec_diff(const timespec_t *a, const timespec_t *b, timespec_t *result)
{
	result->seconds = a->seconds - b->seconds;
	result->nanoseconds = a->nanoseconds - b->nanoseconds;
	if (result->nanoseconds < 0)
	{
		--result->seconds;
		result->nanoseconds += 1000000000L;
	}
}