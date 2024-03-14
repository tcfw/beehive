#include <kernel/uaccess.h>
#include <kernel/cls.h>
#include <kernel/strings.h>

extern int _copy_from_user(void *src, void *dst, uint64_t size);
extern int _copy_to_user(void *src, void *dst, uint64_t size);
extern void mark_failed_copy(void);

// Copy from user space to a kernel space
int copy_from_user(void *src, void *dst, size_t size)
{
	cls_t *cls = get_cls();
	cls->cfe = EXCEPTION_USER_COPY_FROM;
	cls->cfe_handle = mark_failed_copy;

	int ret = -1;

	if ((cls->rq.current_thread->flags & THREAD_KTHREAD) != 0) {
		memcpy(dst, src, size);
		ret = 0;
	} else {
		ret = _copy_from_user(src, dst, (uint64_t)size);
	}

	cls->cfe = EXCEPTION_UNKNOWN;
	cls->cfe_handle = 0;

	return ret;
}

// Copy to user space from a kernel buffer
int copy_to_user(void *src, void *dst, size_t size)
{
	cls_t *cls = get_cls();
	cls->cfe = EXCEPTION_USER_COPY_TO;
	cls->cfe_handle = mark_failed_copy;

	int ret = -1;
	
	if ((cls->rq.current_thread->flags & THREAD_KTHREAD) != 0) {
		memcpy(dst, src, size);
		ret = 0;
	} else {
		ret = _copy_to_user(src, dst, (uint64_t)size);
	}

	cls->cfe = EXCEPTION_UNKNOWN;
	cls->cfe_handle = 0;

	return ret;
}