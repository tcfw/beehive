#include <kernel/rand.h>
#include <kernel/stdint.h>

static uint128_t next = 1;

const uint128_t pcgM = (__int128_t)0x2360ed051fc65da4ULL << 64 | (__uint128_t)0x4385df649fccf645ULL;
const uint128_t pcgA = (__int128_t)0x5851f42d4c957f2dULL << 64 | (__uint128_t)0x14057b7ef767814fULL;

void srand(uint64_t seed)
{
	next = seed;
}

static uint64_t scramble(uint128_t x) {
    uint64_t hi = x>>64;
	uint64_t lo = x;
    hi ^= hi >> 32;
    hi *= 0xda942042e4dd58b5;
    hi ^= hi >> 48;
    hi *= lo | 1;

	return hi;
}

uint64_t rand(void)
{
	next = next * pcgM + pcgA;
	return scramble(next) % ((unsigned long)RAND_MAX + 1);
}