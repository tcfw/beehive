#include <tests/tests.h>
#include <kernel/futex.h>

/*
* Note these tests do not test concurrency within futex locks
*/

NAMED_TEST("futex_key", test_futex_key) {
	union futex_key k={.both.ptr=0x1};

	uint64_t h=futex_hash_key(&k);

	if (h==0)
		TEST_FAIL_MSG("hash produced zero")

	TEST_PASS
}

NAMED_TEST("futex_hb", test_futex_hb) {
	union futex_key k={.both.ptr=0x1};

	futex_hb_t *hb=futex_hb(&k);
	
	if (hb==0)
		TEST_FAIL_MSG("hash bucket zero")

	TEST_PASS
}

NAMED_TEST("futex_sleep", test_futex_sleep) {
	int val=0;
	thread_t *t = create_kthread(NULL, "test", NULL);
	set_current_thread(t);

	union futex_key k = {.both={.ptr=0,.word=0}};
	
	if (futex_get_key(&val, &k)!=0)
		TEST_FAIL_MSG("futex_get_key unexpected result")

	int ret = futex_do_sleep(&val, 1, NULL);
	if (ret!=0) 
		TEST_FAIL_MSG("futex_do_sleep unexpected return result");
	
	if (t->state!=THREAD_SLEEPING) 
		TEST_FAIL_MSG("thread state should be sleeping after futex");
	
	if (t->wc->type!=WAIT) 
		TEST_FAIL_MSG("thread wait cond should be WAIT after futex");

	futex_queue_t *q=((struct thread_wait_cond_futex*)t->wc)->queue;

	if(q->wanted_value!=1)
		TEST_FAIL_MSG("futex_q holds unexpected wanted_value value")

	mark_zombie_thread(t);

	TEST_PASS
}

NAMED_TEST("futex_wake", test_futex_wake) {
	int val=2; //since this is on the stack, need to make sure it's value is something else to ensure an older
	//thread not cleared up yet doesn't get woken up
	//TODO(tcfw) free zombie threads after each test
	thread_t *t1 = create_kthread(NULL, "test1", NULL);
	thread_t *t2 = create_kthread(NULL, "test2", NULL);
	set_current_thread(t1);

	int ret = futex_do_sleep(&val, 5, NULL);
	if (ret!=0) 
		TEST_FAIL_MSG("futex_do_sleep unexpected return result");

	if (t1->state != THREAD_SLEEPING)
		TEST_FAIL_MSGF("t1 unexpected state. was expecting SLEEPING(1) got %d", t1->state);

	set_current_thread(t2);

	val=5;
	ret = futex_do_wake(&val, 5, val);
	if (ret!=1) 
		TEST_FAIL_MSGF("futex_do_wake unexpected return result, was expecting 1 thread to have woken, got %d", ret);

	if (t1->state != THREAD_RUNNING)
		TEST_FAIL_MSGF("t1 unexpected state. was expecting RUNNING(0) got %d", t1->state);

	if (t2->state != THREAD_RUNNING)
		TEST_FAIL_MSGF("t2 state should not have changed, got %d", t2->state);

	mark_zombie_thread(t1);
	mark_zombie_thread(t2);

	TEST_PASS
}