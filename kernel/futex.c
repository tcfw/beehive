#include <errno.h>
#include <kernel/clock.h>
#include <kernel/cls.h>
#include <kernel/futex.h>
#include <kernel/hash.h>
#include <kernel/list.h>
#include <kernel/mm.h>
#include <kernel/sync.h>
#include <kernel/syscall.h>
#include <kernel/uaccess.h>
#include <kernel/vm.h>
#include <kernel/wait.h>

futex_hb_t *futex_buckets;

void futex_init(void)
{
	futex_buckets = (futex_hb_t *)page_alloc_s(sizeof(futex_hb_t) * FUTEX_HASHBUCKETS_SIZE);

	futex_hb_t *f = futex_buckets;
	for (int i = 0; i < FUTEX_HASHBUCKETS_SIZE; i++)
	{
		spinlock_init(&f->lock);
		INIT_LIST_HEAD(&f->chain);
		f++;
	}
}

futex_hb_key futex_hash_key(union futex_key *key)
{
	return hash(key, sizeof(union futex_key), FUTEX_HASH_SEED);
}

futex_hb_t *futex_hb(union futex_key *key)
{
	futex_hb_key k = futex_hash_key(key);

	return futex_buckets + (k % FUTEX_HASHBUCKETS_SIZE);
}

int futex_get_key(void *uaddr, union futex_key *key)
{
	int shared = current_vm_region_shared((uintptr_t)uaddr, sizeof(uint32_t));

	if (!shared)
	{
		key->private.vm = &current->process->vm;
		key->private.addr = (uintptr_t)uaddr;
		return 0;
	}

	key->shared.addr = vm_va_to_pa(current->process->vm.vm_table, (uintptr_t)uaddr);
	key->shared._tmp = 0;

	return 0;
}

int futex_do_wake(void *uaddr, uint32_t n_wake, uint32_t val)
{
	futex_hb_t *hb;
	union futex_key key = {.both = {.ptr = 0ULL}};
	futex_queue_t *queued_task, *next;
	LIST_HEAD(to_wake);
	int ret;

	ret = futex_get_key(uaddr, &key);
	if (ret != 0)
		return ret;

	hb = futex_hb(&key);

	memory_barrier;

	spinlock_acquire(&hb->lock);

	list_head_for_each_safe(queued_task, next, &hb->chain)
	{
		if (!futex_keys_match(&key, &queued_task->key))
			continue;

		if (queued_task->wanted_value != val)
			continue;

		list_del(queued_task);
		list_add(&queued_task->list, &to_wake);

		if (++ret >= n_wake)
			break;
	}

	spinlock_release(&hb->lock);

	list_head_for_each_safe(queued_task, next, &to_wake)
	{
		wake_thread(queued_task->thread);

		list_del(queued_task);
		kfree(queued_task);
	}

	return ret;
}

int futex_do_sleep(void *uaddr, uint32_t val, timespec_t *timeout)
{
	futex_hb_t *hb;
	union futex_key key = {.both = {.ptr = 0ULL}};
	volatile uint32_t uval;
	int ret;

	ret = futex_get_key(uaddr, &key);
	if (ret != 0)
		return ret;

	hb = futex_hb(&key);

	spinlock_acquire(&hb->lock);

	memory_barrier;
	ret = copy_from_user(uaddr, &uval, sizeof(uval));
	if (ret < 0)
	{
		spinlock_release(&hb->lock);
		return -ERRAGAIN;
	}

	if (uval != val)
	{
		spinlock_release(&hb->lock);
		return 0;
	}

	thread_t *thread = current;

	futex_queue_t *queue = kmalloc(sizeof(futex_queue_t));
	if (queue == NULL)
	{
		spinlock_release(&hb->lock);
		return -ERRNOMEM;
	}
	queue->key.both = key.both;
	queue->wanted_value = val;
	queue->thread = thread;

	struct thread_wait_cond_futex *wc = kmalloc(sizeof(struct thread_wait_cond_futex));
	if (wc == NULL)
	{
		spinlock_release(&hb->lock);
		goto freeQueue;
	}

	wc->cond.type = WAIT;
	wc->queue = queue;

	thread_wait_for_cond(thread, wc);
	list_add_tail(&queue->list, &hb->chain);

	spinlock_release(&hb->lock);

	if (timeout == NULL)
		return ret;

	timespec_t *t = kmalloc(sizeof(*t));
	if (t == NULL)
		goto freeWC;

	wc->timeout = t;

	ret = copy_from_user(timeout, t, sizeof(*t));
	if (ret < 0)
		goto freeToT;

	waitqueue_entry_t *wqe = kmalloc(sizeof(waitqueue_entry_t));
	if (wqe == NULL)
		goto freeToT;

	wqe->thread = thread;
	wqe->func = wq_can_wake_thread;
	wqe->timeout = t;

	cls_t *cls = get_cls();
	spinlock_acquire(&cls->sleepq.lock);
	list_add_tail(&wqe->list, &cls->sleepq.head);
	spinlock_release(&cls->sleepq.lock);

	return 0;

freeToT:
	kfree(t);
freeWC:
	thread->wc = NULL;
	kfree(wc);
freeQueue:
	spinlock_acquire(&hb->lock);
	list_del(queue);
	spinlock_release(&hb->lock);
	kfree(queue);
	set_thread_state(thread, THREAD_RUNNING);

	return -ERRNOMEM;
}

DEFINE_SYSCALL5(syscall_futex, SYSCALL_FUTEX, void *, addr, int, op, uint32_t, val, uint32_t, val2, timespec_t *, timeout)
{
	int ret = access_ok(ACCESS_TYPE_READ, addr, sizeof(val));
	if (ret < 0)
		return ret;

	if (timeout != NULL)
	{
		ret = access_ok(ACCESS_TYPE_READ, timeout, sizeof(*timeout));
		if (ret < 0)
			return ret;
	}

	switch (op)
	{
	case FUTEX_OP_WAKE:
		return futex_do_wake(addr, val, val2);
	case FUTEX_OP_SLEEP:
		return futex_do_sleep(addr, val, timeout);
	default:
		return -ERRINVAL;
	}
}