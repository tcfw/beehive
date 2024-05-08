#include <kernel/rand.h>
#include <tests/tests.h>

NAMED_TEST("rand_next", test_rand_next)
{
	uint64_t a=rand();
	uint64_t b=rand();

	if (a==b) {
		TEST_FAIL_MSG("subsequent rand read was equal")
	}

	TEST_PASS
}