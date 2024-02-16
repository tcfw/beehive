#include <tests/tests.h>
#include <kernel/queue.h>
#include <kernel/thread.h>
#include <kernel/unistd.h>

NAMED_TEST("mq_open_id", test_mq_open_id)
{
	thread_t *t = create_kthread(NULL, "test", NULL);
	struct mq_open_params params = {};

	int ret = syscall_mq_open(t, &params);

	if (ret <= 0)
		TEST_FAIL_MSG("unexpected mq_open result");

	if (list_empty(t->queues))
		TEST_FAIL_MSG("thread has no queue refs");

	if (queues_get_skl()->size != 1)
		TEST_FAIL_MSG("queues skl should have 1 queue");

	syscall_mq_close(t, ret);
}