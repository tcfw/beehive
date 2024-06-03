#include "errno.h"
#include <kernel/clock.h>
#include <kernel/list.h>
#include <kernel/mm.h>
#include <kernel/queue.h>
#include <kernel/skiplist.h>
#include <kernel/stdint.h>
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

queue_list_entry_t *queues_find_by_entry(queue_list_entry_t *sq)
{
	return (queue_list_entry_t *)skl_search(&queues, &sq, named_queues_comparator);
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

static void free_mq_buffers(queue_t *queue)
{
	struct list_head *pos;
	struct list_head *tmp;
	list_for_each_safe(pos, tmp, &queue->buffer)
	{
		queue_buffer_list_entry_t *buf = (queue_buffer_list_entry_t *)pos;
		spinlock_acquire(&buf->buffer->lock);
		buf->buffer->refs--;
		spinlock_release(&buf->buffer->lock);

		if (buf->buffer->refs == 0)
		{
			kfree(buf->buffer);
		}

		list_del(buf);
		kfree(buf);
	}
}

DEFINE_SYSCALL1(syscall_mq_open, SYSCALL_MQ_OPEN, const struct mq_open_params *, params)
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
		if (nq != 0 && nq->owner != thread->process->pid)
			// Only the owner should be able to create queues off an existing named queue
			return -ERREXISTS;
	}

	if (params->max_msg_size > MAX_MQ_MSG_SIZE)
		return -ERRSIZE;

	uint32_t id = next_queue_id();
	if (id == 0)
		return -ERREXHAUSTED;

	queue_t *queue = kmalloc(sizeof(queue_t));

	queue->max_msg_size = MAX_MQ_MSG_SIZE;
	if (params->max_msg_size != 0)
		queue->max_msg_size = params->max_msg_size;

	queue->id = id;
	queue->flags = params->flags;
	queue->thread = thread;
	queue->max_msg_count = MAX_MQ_MSG_COUNT;

	INIT_WAITQUEUE(&queue->waiters);
	INIT_LIST_HEAD(&queue->buffer);

	queue_ref_t *ref = kmalloc(sizeof(queue_ref_t));
	ref->queue = queue;
	list_add(&ref->list, &thread->process->queues);

	if (nq == 0)
	{
		nq = kmalloc(sizeof(*nq));
		INIT_LIST_HEAD(&nq->queues);

		nq->owner = thread->process->pid;
		nq->id = queue->id;
		if (params->name[0] != 0)
			memcpy(&nq->name, &params->name, MAX_MQ_NAME_SIZE);

		skl_insert(&queues, nq);
	}

	queue->entry = nq;

	spinlock_acquire(&nq->lock);
	list_add_tail(&queue->list, &nq->queues);
	spinlock_release(&nq->lock);

	return (uint64_t)queue->id;
}

DEFINE_SYSCALL1(syscall_mq_close, SYSCALL_MQ_CLOSE, const uint32_t, id)
{
	queue_ref_t *ref;
	queue_t *tmp = NULL;
	queue_t *queue = NULL;

	list_head_for_each(ref, &thread->process->queues) if (ref->queue->id == id)
	{
		queue = ref->queue;
		break;
	}

	if (queue == NULL)
		return -ERRFAULT;

	queue_list_entry_t *nq = queue->entry;
	if (nq->owner != thread->process->pid)
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
		list_head_for_each(tmp, &nq->queues)
		{
			count++;
			if (count > 1)
				break;
		}

		spinlock_release(&nq->lock);
		if (count != 1)
			goto free_queue;
	}

free_nq:
	skl_delete(&queues, nq);
	kfree(nq);

free_queue:
	// TODO(tcfw) inform waiters queue no longer exists

	list_del((struct list_head *)ref);
	kfree(ref);

	list_del((struct list_head *)queue);
	free_mq_buffers(queue);
	kfree(queue);

	return 0;
}

DEFINE_SYSCALL3(syscall_mq_ctrl, SYSCALL_MQ_CTRL, const uint32_t, id, enum MQ_CTRL_OP, op, uint64_t, data)
{
	queue_ref_t *ref = NULL;
	queue_t *queue = NULL;

	list_head_for_each(ref, &thread->process->queues) if (ref->queue->id == id)
	{
		queue = ref->queue;
		break;
	}

	if (queue == 0)
		return -ERRFAULT;

	if (op >= MQ_CTRL_OP_MAX)
		return -ERRSIZE;

	if (op == MQ_CTRL_OP_MAX_MSG_COUNT)
	{
		if (data > MAX_MQ_MSG_COUNT)
			return -ERRSIZE;

		spinlock_acquire(&queue->lock);
		queue->max_msg_count = data;
		spinlock_release(&queue->lock);
	}
	else if (op == MQ_CTRL_OP_MAX_MSG_SIZE)
	{
		if (data > MAX_MQ_MSG_SIZE)
			return -ERRSIZE;

		spinlock_acquire(&queue->lock);
		queue->max_msg_size = data;
		spinlock_release(&queue->lock);
	}
	else if (op == MQ_CTRL_OP_SET_PERMISSIONS)
	{
		spinlock_acquire(&queue->lock);
		queue->permissions = (uint16_t)data;
		spinlock_release(&queue->lock);
	}

	try_wake_waitqueue(&queue->waiters);

	return 0;
}

DEFINE_SYSCALL3(syscall_mq_send, SYSCALL_MQ_SEND, const struct mq_send_params *, params, const void *, data, const size_t, dlen)
{
	int ok = access_ok(ACCESS_TYPE_READ, data, dlen);
	if (ok < 0)
		return ok;

	ok = access_ok(ACCESS_TYPE_READ, params, sizeof(struct mq_send_params));
	if (ok < 0)
		return ok;

	// TODO(tcfw) permissions

	queue_list_entry_t *entry;
	queue_t *queue = NULL;

	queue_list_entry_t *sq = kmalloc(sizeof(queue_list_entry_t));
	copy_from_user(&params->name, &sq->name, MAX_MQ_NAME_SIZE);
	if (sq->name[0] == 0)
	{
		copy_from_user(&params->id, &sq->id, sizeof(sq->id));
	}

	entry = queues_find_by_entry(sq);
	kfree(sq);

	if (entry == NULL)
		return -ERRFAULT;

	spinlock_acquire(&entry->lock);

	uint32_t refCount = 0;

	list_head_for_each(queue, &entry->queues)
	{
		if (dlen > queue->max_msg_size)
		{
			spinlock_release(&entry->lock);
			return -ERRSIZE;
		}

		refCount++;
	}

	queue_buffer_t *buf;
	if (dlen < PAGE_SIZE)
	{
		buf = kmalloc(sizeof(*buf) + dlen);
	}
	else
	{
		buf = (queue_buffer_t *)page_alloc_s(sizeof(queue_buffer_t) + dlen);
	}

	buf->len = dlen;
	buf->refs = refCount;
	buf->sender = thread->process->pid;

	struct clocksource_t *cs = clock_first(CS_GLOBAL);
	timespec_from_cs(cs, &buf->recv);
	spinlock_init(&buf->lock);
	copy_from_user(data, buf->buf, dlen);

	spinlock_acquire(&queue->lock);

	struct thread_wait_cond_queue_io *wc = NULL;

	list_head_for_each(queue, &entry->queues)
	{
		if (list_len(&queue->buffer) >= queue->max_msg_count)
		{
			if (wc == NULL)
			{
				wc = kmalloc(sizeof(*wc));
				INIT_LIST_HEAD(&wc->queues);
				wc->buf = buf;
				wc->cond.type = QUEUE_IO;
				wc->flags = THREAD_QUEUE_IO_WRITE;
			}

			queue_ref_t *qr = kmalloc(sizeof(queue_ref_t));
			qr->queue = queue;

			list_add(&qr->list, &wc->queues);

			waitqueue_entry_t *wqe = kmalloc(sizeof(waitqueue_entry_t));
			wqe->thread = thread;
			wqe->func = wq_can_wake_thread;
			wqe->data = (void *)buf;

			spinlock_acquire(&queue->waiters.lock);
			list_add_tail(&wqe->list, &queue->waiters.head);
			spinlock_release(&queue->waiters.lock);
		}
		else
		{
			queue_buffer_list_entry_t *bufle = kmalloc(sizeof(queue_buffer_list_entry_t));
			bufle->buffer = buf;

			list_add_tail(&bufle->list, &queue->buffer);
		}

		spinlock_release(&queue->lock);
	}

	if (wc != NULL)
		thread->wc = wc;

	list_head_for_each(queue, &entry->queues)
	{
		try_wake_waitqueue(&queue->waiters);
	}

	spinlock_release(&entry->lock);

	if (wc != NULL)
		thread_wait_for_cond(thread, wc);

	return 0;
}

DEFINE_SYSCALL3(syscall_mq_recv, SYSCALL_MQ_RECV, const uint32_t, id, void *, data, const size_t, dlen)
{
	int ok = access_ok(ACCESS_TYPE_WRITE, data, dlen);
	if (ok < 0)
		return ok;

	// TODO(tcfw) permissions

	queue_ref_t *ref = NULL;
	queue_t *queue = NULL;

	list_head_for_each(ref, &thread->process->queues) if (ref->queue->id == id)
	{
		queue = ref->queue;
		break;
	}

	if (queue == 0)
		return -ERRFAULT;

	if (queue->max_msg_size > dlen)
		return -ERRSIZE;

	spinlock_acquire(&queue->lock);

	if (list_is_empty(&queue->buffer))
	{
		spinlock_release(&queue->lock);

		// TODO(tcfw) determine block or async

		waitqueue_entry_t *wqe = kmalloc(sizeof(waitqueue_entry_t));
		wqe->thread = thread;
		wqe->func = wq_can_wake_thread;
		list_add_tail(&wqe->list, &queue->waiters);

		struct thread_wait_cond_queue_io *wc = kmalloc(sizeof(struct thread_wait_cond_queue_io));
		INIT_LIST_HEAD(&wc->queues);
		wc->cond.type = QUEUE_IO;
		wc->flags = THREAD_QUEUE_IO_READ;

		queue_ref_t *qr = kmalloc(sizeof(queue_ref_t));
		qr->queue = queue;
		list_add(&qr->list, &wc->queues);

		thread_wait_for_cond(thread, wc);

		return 0;
	}

	queue_buffer_list_entry_t *buf = (queue_buffer_list_entry_t *)queue->buffer.next;
	size_t len = buf->buffer->len;

	list_del(buf);
	spinlock_release(&queue->lock);

	copy_to_user((void *)&buf->buffer->buf, data, len);

	spinlock_acquire(&buf->buffer->lock);
	buf->buffer->refs--;
	spinlock_release(&buf->buffer->lock);

	if (buf->buffer->refs == 0)
	{
		if (len < PAGE_SIZE)
		{
			kfree(buf->buffer);
		}
		else
		{
			page_free(buf->buffer);
		}
	}

	return (uint64_t)len;
}