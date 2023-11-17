#include <kernel/tty.h>
#include <kernel/panic.h>
#include <tests/tests.h>

void run_self_tests(void)
{
	terminal_log("STARTING SELF-RUN TESTS");

	for (test_func_ptr_t *test =
			 (test_func_ptr_t *)({
				 extern test_func_ptr_t __start_tests;
				 &__start_tests;
			 });
		 test !=
		 (test_func_ptr_t *)({
			 extern test_func_ptr_t __stop_tests;
			 &__stop_tests;
		 });
		 ++test)
	{
		int res = test->callback();

		static char *pf;
		if (res < 0)
			pf = "FAIL";
		else
			pf = "PASS";

		terminal_logf("%s %s", pf, test->test_desc);
	}

	panic("FINISHED SELF-RUN TESTS");
}