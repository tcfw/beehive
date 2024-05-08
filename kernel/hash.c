#include <kernel/hash.h>

#define XXH_STATIC_LINKING_ONLY /* access advanced declarations */
#define XXH_IMPLEMENTATION      /* access definitions */

#include <kernel/xxhash.h>

uint64_t hash(const void *data, size_t len, uint64_t seed) {
	return xxh64(data, len, seed);
}