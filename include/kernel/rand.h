#ifndef _KERNEL_RAND_H
#define _KERNEL_RAND_H

#include <kernel/stdint.h>

// #define RAND_MAX (2147483647)
#define RAND_MAX (0xFFFFFFFFFFFFFFFFULL)

// pseudo-random number
uint64_t rand(void);

// seed the pseudo-random number generator
void srand(uint64_t seed);

#endif