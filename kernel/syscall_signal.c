#include <errno.h>
#include <kernel/signal.h>
#include <kernel/syscall.h>
#include <kernel/thread.h>
#include <kernel/tty.h>
#include <kernel/uaccess.h>
#include <kernel/strings.h>
#include <kernel/vm.h>

DEFINE_SYSCALL2(syscall_sig_tstack, SYSCALL_SIG_TSTACK, stack_t *, new, stack_t *, old)
{
	int ret = 0;
	stack_t newss = {};
	stack_t oldss = {};

	if (old == NULL && new == NULL)
		return -ERRFAULT;

	memcpy(&oldss, &thread->sigactions.sig_stack, sizeof(oldss));

	if (new != NULL)
	{
		ret = access_ok(ACCESS_TYPE_READ, new, sizeof(*new));
		if (ret < 0)
			return ret;

		ret = copy_from_user(new, &newss, sizeof(newss));
		if (ret < 0)
			return ret;

		if (newss.sp == 0)
			return -ERRINVALID;

		if (newss.size == 0)
			newss.size = MINSIGSTKSIZ;

		if (newss.size < MINSIGSTKSIZ)
			return -ERRNOMEM;

		thread->sigactions.sig_stack.flags = newss.flags;
		thread->sigactions.sig_stack.sp = newss.sp;
		thread->sigactions.sig_stack.size = newss.size;
	}

	if (old != NULL)
	{
		ret = access_ok(ACCESS_TYPE_WRITE, old, sizeof(*old));
		if (ret < 0)
			return ret;

		ret = copy_to_user(&thread->sigactions.sig_stack, &oldss, sizeof(oldss));
		if (ret < 0)
			return ret;
	}

	return 0;
}