#include <kernel/mm.h>
#include <kernel/syscall.h>
#include <kernel/thread.h>
#include <kernel/tty.h>
#include <kernel/uaccess.h>

int writeconsole(thread_t *t, void *c, size_t n)
{
	if (n > 40960)
		return -2;

	int aok = access_ok(ACCESS_TYPE_READ, c, n);
	if (aok < 0)
		return aok;

	void *buf = page_alloc_s(n + 1);

	if (copy_from_user(c, buf, n) < 0)
	{
		page_free(buf);
		return -1;
	}

	*(char *)(buf + n + 1) = 0;

	terminal_log((char *)buf);

	page_free(buf);
	return 0;
}

SYSCALL(131, writeconsole, 2);