#ifndef _KERNEL_HASH_H
#define _KERNEL_HASH_H

#include <kernel/stdint.h>
#include <kernel/unistd.h>

uint64_t hash(const void *data, size_t len, uint64_t seed);

#endif