#include <kernel/uaccess.h>

extern int _copy_from_user(void *src, void *dst, uint64_t size);
extern int _copy_to_user(void *src, void *dst, uint64_t size);

// Copy from user space to a kernel space
int copy_from_user(void *src, void *dst, size_t size)
{
	return _copy_from_user(src, dst, (uint64_t)size);
}

// Copy to user space from a kernel buffer
int copy_to_user(void *src, void *dst, size_t size)
{
	return _copy_to_user(src, dst, (uint64_t)size);
}