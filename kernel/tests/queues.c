#include <kernel/cls.h>
#include <kernel/mm.h>
#include <kernel/queue.h>
#include <kernel/strings.h>
#include <kernel/thread.h>
#include <kernel/unistd.h>
#include <tests/tests.h>

NAMED_TEST("mq_open_id", test_mq_open_id)
{
	thread_t *t = create_kthread(NULL, "test", NULL);
	set_current_thread(t);

	struct mq_open_params params = {};

	int ret = syscall_mq_open(t, &params);

	if (ret <= 0)
	{
		terminal_logf("unexpected mq_open result, got %d, was expecting 0", ret);
		free_thread(t);
		TEST_FAIL
	}

	if (list_empty(t->queues))
	{
		free_thread(t);
		TEST_FAIL_MSG("thread has no queue refs");
	}

	if (queues_get_skl()->size != 1)
	{
		free_thread(t);
		TEST_FAIL_MSG("queues skl should have 1 queue");
	}

	if (syscall_mq_close(t, ret) < 0)
	{
		free_thread(t);
		TEST_FAIL_MSG("failed to clean queue");
	}

	free_thread(t);

	TEST_PASS
}

NAMED_TEST("mq_open_named", test_mq_open_named)
{
	thread_t *t = create_kthread(NULL, "test", NULL);
	set_current_thread(t);

	struct mq_open_params params = {
		.name = "mq_open_named",
	};

	int ret = syscall_mq_open(t, &params);

	if (ret <= 0)
	{
		terminal_logf("unexpected mq_open result, got %d, was expecting 0", ret);
		free_thread(t);
		TEST_FAIL
	}

	if (list_empty(t->queues))
	{
		free_thread(t);
		TEST_FAIL_MSG("thread has no queue refs");
	}

	queue_list_entry_t sq = {.name = "mq_open_named"};

	queue_list_entry_t *nq = queues_find_by_entry(&sq);
	if (nq == NULL)
	{
		free_thread(t);
		TEST_FAIL_MSG("no named queue created");
	}

	if (queues_get_skl()->size != 1)
	{
		free_thread(t);
		TEST_FAIL_MSG("queues skl should have 1 queue");
	}

	if (syscall_mq_close(t, ret) < 0)
	{
		free_thread(t);
		TEST_FAIL_MSG("failed to clean queue");
	}

	free_thread(t);

	TEST_PASS
}

NAMED_TEST("mq_send_named", test_mq_send_named)
{
	thread_t *t = create_kthread(NULL, "test", NULL);
	set_current_thread(t);

	struct mq_open_params params = {
		.name = "mq_send_named",
	};

	int qid = syscall_mq_open(t, &params);

	if (qid <= 0)
	{
		terminal_logf("unexpected mq_open result, got %d, was expecting 0", qid);
		free_thread(t);
		TEST_FAIL
	}

	struct mq_send_params send_params = {
		.name = "mq_send_named"};

	char *data = "test";
	size_t len = sizeof(data);

	int ret = syscall_mq_send(t, &send_params, data, len);
	if (ret < 0)
	{
		terminal_logf("unexpected mq_send result, got %d, was expecting 0", ret);
		free_thread(t);
		TEST_FAIL
	}

	queue_list_entry_t sq = {.name = "mq_send_named"};

	queue_list_entry_t *nq = queues_find_by_entry(&sq);
	if (nq == NULL)
	{
		free_thread(t);
		TEST_FAIL_MSG("couldn't find related queue");
	}

	if (list_empty(((queue_t *)nq->queues.next)->buffer))
	{
		free_thread(t);
		TEST_FAIL_MSG("queue buffer was empty");
	}

	if (syscall_mq_close(t, qid) < 0)
	{
		free_thread(t);
		TEST_FAIL_MSG("failed to clean queue");
	}

	free_thread(t);
	TEST_PASS
}

NAMED_TEST("mq_send_named_max_msg_count", test_mq_send_named_max_msg_count)
{
	thread_t *t = create_kthread(NULL, "test", NULL);
	set_current_thread(t);

	struct mq_open_params params = {
		.name = "mq_send_named_max_msg_count",
	};

	int qid = syscall_mq_open(t, &params);

	if (qid <= 0)
	{
		terminal_logf("unexpected mq_open result, got %d, was expecting 0", qid);
		free_thread(t);
		TEST_FAIL
	}

	int ret = syscall_mq_ctrl(t, qid, MQ_CTRL_OP_MAX_MSG_COUNT, 1);
	if (ret < 0)
	{
		terminal_logf("unexpected mq_ctrl result, got %d, was expecting 0", ret);
		free_thread(t);
		TEST_FAIL
	}

	struct mq_send_params send_params = {
		.name = "mq_send_named_max_msg_count"};

	char *data = "test";
	size_t len = strlen(data) + 1;

	// send 1
	ret = syscall_mq_send(t, &send_params, data, len);
	if (ret < 0)
	{
		terminal_logf("unexpected mq_send result, got %d, was expecting 0", ret);
		free_thread(t);
		TEST_FAIL
	}

	// send 2
	ret = syscall_mq_send(t, &send_params, data, len);
	if (ret < 0)
	{
		terminal_logf("unexpected mq_send result, got %d, was expecting 0", ret);
		free_thread(t);
		TEST_FAIL
	}

	queue_list_entry_t sq = {.name = "mq_send_named"};
	queue_list_entry_t *nq = queues_find_by_entry(&sq);
	if (nq == NULL)
	{
		free_thread(t);
		TEST_FAIL_MSG("couldn't find related queue");
	}

	if (list_empty(((queue_t *)nq->queues.next)->buffer))
	{
		free_thread(t);
		TEST_FAIL_MSG("queue buffer was empty");
	}

	if (t->state != SLEEPING)
	{
		free_thread(t);
		TEST_FAIL_MSG("thread should have gone to sleep");
	}

	if (syscall_mq_close(t, qid) < 0)
	{
		free_thread(t);
		TEST_FAIL_MSG("failed to clean queue");
	}

	free_thread(t);
	TEST_PASS
}

NAMED_TEST("mq_recv_named", test_mq_recv_named)
{
	thread_t *t = create_kthread(NULL, "test", NULL);
	set_current_thread(t);

	struct mq_open_params params = {
		.name = "mq_recv_named",
	};

	int qid = syscall_mq_open(t, &params);

	if (qid <= 0)
	{
		terminal_logf("unexpected mq_open result, got %d, was expecting 0", qid);
		free_thread(t);
		TEST_FAIL
	}

	struct mq_send_params send_params = {
		.name = "mq_recv_named"};

	char *data = "test";
	size_t len = strlen(data) + 1;

	// send 1
	int ret = syscall_mq_send(t, &send_params, data, len);
	if (ret < 0)
	{
		terminal_logf("unexpected mq_send result, got %d, was expecting 0", ret);
		free_thread(t);
		TEST_FAIL
	}

	void *recv_buf = page_alloc_s(MAX_MQ_MSG_SIZE);
	memset(recv_buf, 0, MAX_MQ_MSG_SIZE);
	ret = syscall_mq_recv(t, qid, recv_buf, MAX_MQ_MSG_SIZE);
	if (ret < 0)
	{
		terminal_logf("unexpected mq_recv results, got %d, was expected greater than 0", ret);
		free_thread(t);
		TEST_FAIL
	}

	if (strcmp(data, (char *)recv_buf) != 0)
	{
		terminal_logf("unexpected buf comparion, got %s, was expected %s", (char *)recv_buf, data);
		free_thread(t);
		TEST_FAIL
	}

	page_free(recv_buf);

	queue_list_entry_t sq = {.name = "mq_recv_named"};
	queue_list_entry_t *nq = queues_find_by_entry(&sq);
	if (nq == NULL)
	{
		free_thread(t);
		TEST_FAIL_MSG("couldn't find related queue");
	}

	if (!list_empty(((queue_t *)nq->queues.next)->buffer))
	{
		free_thread(t);
		TEST_FAIL_MSG("queue buffer was not empty");
	}

	if (syscall_mq_close(t, qid) < 0)
	{
		free_thread(t);
		TEST_FAIL_MSG("failed to clean queue");
	}

	free_thread(t);
	TEST_PASS
}

NAMED_TEST("mq_cross_thread_send_recv", test_mq_cross_thread_send_recv)
{
	thread_t *t = create_kthread(NULL, "test", NULL);
	set_current_thread(t);

	thread_t *t2 = create_kthread(NULL, "test 2", NULL);

	struct mq_open_params params = {
		.name = "mq_cross_thread_send_recv",
	};

	int qid = syscall_mq_open(t, &params);

	if (qid <= 0)
	{
		terminal_logf("unexpected mq_open result, got %d, was expecting 0", qid);
		free_thread(t);
		TEST_FAIL
	}

	struct mq_send_params send_params = {
		.name = "mq_cross_thread_send_recv"};

	char *data = "test";
	size_t len = strlen(data) + 1;

	// switch to the other kthread
	set_current_thread(t2);
	int ret = syscall_mq_send(t, &send_params, data, len);
	if (ret < 0)
	{
		terminal_logf("unexpected mq_send result, got %d, was expecting 0", ret);
		free_thread(t2);
		free_thread(t);
		TEST_FAIL
	}

	free_thread(t2);

	// switch back to the first thread
	set_current_thread(t);
	void *recv_buf = page_alloc_s(MAX_MQ_MSG_SIZE);
	memset(recv_buf, 0, MAX_MQ_MSG_SIZE);
	ret = syscall_mq_recv(t, qid, recv_buf, MAX_MQ_MSG_SIZE);
	if (ret < 0)
	{
		terminal_logf("unexpected mq_recv results, got %d, was expected greater than 0", ret);
		free_thread(t);
		TEST_FAIL
	}

	if (strcmp(data, (char *)recv_buf) != 0)
	{
		terminal_logf("unexpected buf comparion, got %s, was expected %s", (char *)recv_buf, data);
		free_thread(t);
		TEST_FAIL
	}

	page_free(recv_buf);

	queue_list_entry_t sq = {.name = "mq_cross_thread_send_recv"};
	queue_list_entry_t *nq = queues_find_by_entry(&sq);
	if (nq == NULL)
	{
		free_thread(t);
		TEST_FAIL_MSG("couldn't find related queue");
	}

	if (!list_empty(((queue_t *)nq->queues.next)->buffer))
	{
		free_thread(t);
		TEST_FAIL_MSG("queue buffer was not empty");
	}

	if (syscall_mq_close(t, qid) < 0)
	{
		free_thread(t);
		TEST_FAIL_MSG("failed to clean queue");
	}

	free_thread(t);
	TEST_PASS
}