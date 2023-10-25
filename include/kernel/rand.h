#ifndef _KERNEL_RAND_H
#define _KERNEL_RAND_H

#define RAND_MAX (2147483647)

// pseudo-random number
int rand(void);

// seed the pseudo-random number generator
void srand(int seed);

#endif