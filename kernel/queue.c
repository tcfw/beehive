#include "errno.h"
#include "stdint.h"
#include <kernel/list.h>
#include <kernel/mm.h>
#include <kernel/queue.h>
#include <kernel/skiplist.h>
#include <kernel/strings.h>
#include <kernel/syscall.h>
#include <kernel/thread.h>
#include <kernel/uaccess.h>
#include <kernel/wait.h>

static uint32_t queue_id_counter;
static skiplist_t queues;

skiplist_t *queues_get_skl()
{
	return &queues;
}

static int named_queues_comparator(void *rnode, void *list_rnode)
{
	queue_list_entry_t *rnode_q = (queue_list_entry_t *)rnode;
	queue_list_entry_t *list_rnode_q = (queue_list_entry_t *)list_rnode;

	if (rnode_q->name != 0 && list_rnode_q->name != 0)
		return strcmp(&rnode_q->name, &list_rnode_q->name);

	if (rnode_q->id == list_rnode_q->id)
		return 0;
	else if (rnode_q->id < list_rnode_q->id)
		return -1;
	return 1;
}

static uint32_t next_queue_id()
{
	for (int tries = 0; tries < 5; tries++)
	{
		uint32_t next = queue_id_counter++;

		queue_list_entry_t sq;
		sq.id = next;
		queue_list_entry_t *nq = (queue_list_entry_t *)skl_search(&queues, &sq, named_queues_comparator);
		if (nq == 0)
			return next;
	}

	return 0;
}

void queues_init()
{
	skl_init(&queues, SKIPLIST_DEFAULT_LEVELS, named_queues_comparator, 0);
	queue_id_counter = 1;
}

int syscall_mq_open(thread_t *thread, const struct mq_open_params *params)
{
	int ok = access_ok(ACCESS_TYPE_READ, params, sizeof(struct mq_open_params));
	if (ok < 0)
		return ok;

	queue_list_entry_t *nq = 0;

	if (params->name[0] != 0)
	{
		queue_list_entry_t sq;
		memcpy(&sq.name, &params->name, MAX_MQ_NAME_SIZE);
		nq = (queue_list_entry_t *)skl_search(&queues, &sq, named_queues_comparator);
		if (nq != 0 && nq->owner != thread->pid)
			return -ERREXISTS;
	}

	if (params->max_msg_size > MAX_MQ_MSG_SIZE)
		return -ERRSIZE;

	uint32_t id = next_queue_id();
	if (id == 0)
		return -ERREXHAUSTED;

	queue_t *queue = (queue_t *)kmalloc(sizeof(queue_t));

	queue->max_msg_size = MAX_MQ_MSG_SIZE;
	if (params->max_msg_size != 0)
		queue->max_msg_size = params->max_msg_size;

	queue->id = id;
	queue->flags = params->flags;
	queue->thread = thread;
	queue->max_msg_count = MAX_MQ_MSG_COUNT;

	INIT_WAITQUEUE(&queue->waiters);
	INIT_LIST_HEAD(&queue->buffer);

	queue_ref_t *ref = (queue_ref_t *)kmalloc(sizeof(queue_ref_t));
	ref->queue = queue;
	list_add(ref, &thread->queues);

	if (nq == 0)
	{
		nq = (queue_list_entry_t *)kmalloc(sizeof(queue_list_entry_t));
		INIT_LIST_HEAD(&nq->queues);

		nq->owner = thread->pid;
		nq->id = queue->id;
		if (params->name[0] != 0)
			memcpy(&nq->name, &params->name, MAX_MQ_NAME_SIZE);

		skl_insert(&queues, nq);
	}

	queue->entry = nq;

	spinlock_acquire(&nq->lock);
	list_add_tail(queue, &nq->queues);
	spinlock_release(&nq->lock);

	return queue->id;
}

static free_mq_buffers(queue_t *queue)
{
	struct list_head *pos;
	struct list_head *tmp;
	list_for_each_safe(pos, tmp, &queue->buffer)
	{
		list_del(pos);
		kfree(pos);
	}
}

int syscall_mq_close(thread_t *thread, const uint32_t id)
{
	queue_ref_t *ref;
	queue_t *q;

	list_head_for_each(ref, &thread->queues) if (ref->queue->id == id)
	{
		q = ref->queue;
		break;
	}

	if (q == 0)
		return -ERRFAULT;

	queue_list_entry_t *nq = q->entry;
	if (nq->owner != thread->pid)
	{
		// since we're listening to a named queue, just
		// remove the queue from lists and free the buffers
		goto free_queue;
	}

	if (nq->name[0] != 0)
	{
		spinlock_acquire(&nq->lock);
		// if named queue, check other queues aren't still using it
		int count = 0;
		list_head_for_each(q, &nq->queues)
		{
			count++;
			if (count > 1)
				break;
		}

		spinlock_release(&nq->lock);
		if (count != 1)
			return -ERRINUSE;
	}

free_nq:
	skl_delete(&queues, nq);
	kfree(nq);

free_queue:
	// TODO(tcfw) inform waiters queue no longer exists

	list_del((struct list_head *)ref);
	kfree(ref);

	list_del((struct list_head *)q);
	free_mq_buffers(q);
	kfree(q);

	return 0;
}

int syscall_mq_ctrl(thread_t *thread, const uint32_t id, enum MQ_CTRL_OP op, uint64_t data)
{
	queue_ref_t *ref;
	queue_t *q;

	list_head_for_each(ref, &thread->queues) if (ref->queue->id == id)
	{
		q = ref->queue;
		break;
	}

	if (q == 0)
		return -ERRFAULT;

	if (op >= MQ_CTRL_OP_MAX)
		return -ERRSIZE;

	if (op == MQ_CTRL_OP_MAX_MSG_COUNT)
	{
		if (data > MAX_MQ_MSG_COUNT)
			return -ERRSIZE;

		spinlock_acquire(&q->lock);
		q->max_msg_count = data;
		spinlock_release(&q->lock);
	}
	else if (op == MQ_CTRL_OP_MAX_MSG_SIZE)
	{
		if (data > MAX_MQ_MSG_SIZE)
			return -ERRSIZE;

		spinlock_acquire(&q->lock);
		q->max_msg_size = data;
		spinlock_release(&q->lock);
	}
	else if (op == MQ_CTRL_OP_SET_PERMISSIONS)
	{
		spinlock_acquire(&q->lock);
		q->permissions = (uint16_t)data;
		spinlock_release(&q->lock);
	}

	try_wake_waitqueue(&q->waiters);

	return 0;
}

SYSCALL(SYSCALL_MQ_OPEN, syscall_mq_open, 1);
SYSCALL(SYSCALL_MQ_CLOSE, syscall_mq_close, 1);
SYSCALL(SYSCALL_MQ_CTRL, syscall_mq_ctrl, 3);