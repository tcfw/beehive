#include "errno.h"
#include <kernel/clock.h>
#include <kernel/syscall.h>
#include <kernel/tty.h>
#include <kernel/vm.h>
#include <kernel/uaccess.h>

DEFINE_SYSCALL2(syscall_get_time, SYSCALL_GET_TIME, uint8_t, clock_type, timespec_t *, timeval)
{
	struct clocksource_t *cs;
	switch (clock_type)
	{
	case 0:
		// RTC
		int ok = access_ok(ACCESS_TYPE_WRITE, timeval, sizeof(*timeval));
		if (ok < 0)
			return ok;
		cs = clock_first(CS_RTC);
		if (cs == 0)
			return -ERRNOENT;

		timespec_t val = {
			.seconds = (int64_t)cs->val(cs),
		};
		int ret = copy_to_user(&val, timeval, sizeof(*timeval));
		if (ret < 0)
			return ret;

		return 0;
	case 1:
		// monotomic runtime
		cs = clock_first(CS_GLOBAL);
		if (cs == 0)
			return 0;
		return cs->val(cs);
	default:
		return -ERRINVAL;
	}
}
