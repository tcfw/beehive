#include <kernel/syscall.h>
#include <kernel/tty.h>
#include <kernel/uaccess.h>
#include <kernel/mm.h>

int writeconsole(int _type, void *c, size_t n)
{
	int aok = access_ok(READ, c, n);
	if (aok < 0)
		return aok;

	void *buf = page_alloc_s(n);

	if (copy_from_user(c, buf, n) < 0)
	{
		page_free(buf);
		return -1;
	}

	terminal_write((const char *)buf, n);

	page_free(buf);
	return 0;
}

SYSCALL(131, writeconsole, 2);