#include <kernel/rand.h>

static unsigned long next = 1;

void srand(int seed)
{
	next = seed;
}

int rand(void)
{
	return ((next = next * 1103515245 + 12345) % ((unsigned long)RAND_MAX + 1));
}