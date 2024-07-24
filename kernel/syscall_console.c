#include <kernel/mm.h>
#include <kernel/syscall.h>
#include <kernel/thread.h>
#include <kernel/tty.h>
#include <kernel/strings.h>
#include <kernel/uaccess.h>

DEFINE_SYSCALL2(writeconsole, SYSCALL_CONSOLE_WRITE, const void *, c, size_t, n)
{
	if (n > 40960)
		return -2;

	int aok = access_ok(ACCESS_TYPE_READ, c, n);
	if (aok < 0)
		return aok;

	void *buf = page_alloc_s(n);
	memset(buf, 0, n + (PAGE_SIZE - (n % PAGE_SIZE)));

	int ret = copy_from_user(c, buf, n);
	if (ret < 0)
	{
		page_free(buf);
		return ret;
	}

	// terminal_write(buf, n);
	terminal_logf(buf);

	// if (((char *)buf)[n - 1] == '\n')
	// 	terminal_write("\r", 1);

	page_free(buf);

	return 0;
}