#include <kernel/tty.h>

#ifndef _TESTS_TESTS_H
#define _TESTS_TESTS_H

#ifndef RUN_SELF_TESTS
#define RUN_SELF_TESTS (0)
#endif

typedef int (*test_cb)(void);

typedef struct test_func_ptr_t
{
	test_cb callback;

	char *test_desc;
	unsigned int file_pos;
	char *file_loc;
} test_func_ptr_t;

#define CONCAT(a, b) CONCAT_INNER(a, b)
#define CONCAT_INNER(a, b) a##b
#define UNIQUE_NAME(base) CONCAT(base, __COUNTER__)

#define TEST(desc) \
	NAMED_TEST(desc, UNIQUE_NAME(self_run_test))

#define assert(body) \
	if (!(body))     \
	{                \
		TEST_FAIL    \
	}

#define assert_msg(body, msg) \
	if (!(body))              \
	{                         \
		TEST_FAIL_MSG(msg)    \
	}

#define assert_eq(lhs, rhs) \
	if (!((lhs) == (rhs)))  \
	{                       \
		TEST_FAIL           \
	}

#define assert_eq_msg(lhs, rhs, msg) \
	if (!((lhs) == (rhs)))           \
	{                                \
		TEST_FAIL_MSG(msg)           \
	}

#define assert_neq(lhs, rhs) \
	if (((lhs) == (rhs)))    \
	{                        \
		TEST_FAIL            \
	}

#define assert_neq_msg(lhs, rhs, msg) \
	if (((lhs) == (rhs)))             \
	{                                 \
		TEST_FAIL_MSG(msg)            \
	}

#define assert_list_not_empty_msg(list, msg) \
	if (list_is_empty(list))                 \
	{                                        \
		TEST_FAIL_MSG(msg)                   \
	}

#define assert_list_empty_msg(list, msg) \
	if (!list_is_empty(list))            \
	{                                    \
		TEST_FAIL_MSG(msg)               \
	}

#define NAMED_TEST(desc, func_name)                \
	static int func_name();                        \
	static test_func_ptr_t CONCAT(ptr_, func_name) \
		__attribute((used, section(".tests"))) = { \
			.callback = func_name,                 \
			.test_desc = desc,                     \
			.file_loc = __FILE__,                  \
			.file_pos = __LINE__,                  \
	};                                             \
	static int func_name()

#define TEST_FAIL_MSG(msg) \
	terminal_log(msg);     \
	TEST_FAIL

#define TEST_FAIL \
	return -1;

#define TEST_PASS \
	return 0;

void run_self_tests(void);

#endif