#ifndef _KERNEL_UACCESS_H
#define _KERNEL_UACCESS_H

#include "unistd.h"

// Copy from user space to a kernel space
int copy_from_user(void *src, void *dst, size_t size);

// Copy to user space from a kernel buffer
int copy_to_user(void *src, void *dst, size_t size);

#endif