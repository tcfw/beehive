#ifndef _KERNEL_UACCESS_H
#define _KERNEL_UACCESS_H

#include "unistd.h"

enum AccessType
{
	READ = 0,
	WRITE = 1,
};

// Copy from user space to a kernel space
int copy_from_user(void *src, void *dst, size_t size);

// Copy to user space from a kernel buffer
int copy_to_user(void *src, void *dst, size_t size);

// Checks if the user space is allowed by the current task
int access_ok(enum AccessType type, void *addr, size_t n);

#endif