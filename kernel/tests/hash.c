#include <kernel/hash.h>
#include <tests/tests.h>

NAMED_TEST("hash_raw_bytes", test_hash_raw_bytes)
{
	char d[2]={'a','a'};
	uint64_t out = hash(&d, sizeof(d), 0);
	if (out != 0xf646a88e7707a4e9) {
		TEST_FAIL_MSG("xxhash wrong test value")
	}

	TEST_PASS
}