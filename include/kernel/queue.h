#ifndef _KERNEL_QUEUE_H
#define _KERNEL_QUEUE_H

#include <kernel/stdint.h>
#include <kernel/list.h>
#include <kernel/sync.h>
#include <kernel/thread.h>
#include <kernel/wait.h>

#define MAX_MQ_MSG_SIZE (PAGE_SIZE)
#define MAX_MQ_NAME_SIZE (50)
#define MAX_MQ_MSG_COUNT (100)

enum MQ_CTRL_OP
{
	MQ_CTRL_OP_MAX_MSG_COUNT,
	MQ_CTRL_OP_MAX_MSG_SIZE,
	MQ_CTRL_OP_SET_PERMISSIONS,
	MQ_CTRL_OP_MAX,
};

struct mq_open_params
{
	uint32_t flags;
	uint64_t max_msg_size;
	char name[MAX_MQ_NAME_SIZE];
};

typedef struct queue_msg_t queue_msg_t;

typedef struct queue_msg_t
{
	struct list_head list;
	size_t size;
	char *data;

	uint64_t created;
	pid_t creator;
} queue_msg_t;

typedef struct queue_list_entry_t queue_list_entry_t;

typedef struct queue_t
{
	struct list_head list;

	queue_list_entry_t *entry;

	uint32_t id;
	uint32_t flags;
	waitqueue_head_t waiters;
	thread_t *thread;

	spinlock_t lock;
	struct list_head buffer;

	uint64_t received_msgs;
	uint64_t max_msg_size;
	uint64_t max_msg_count;

	uint16_t permissions;
} queue_t;

typedef struct queue_ref_t
{
	struct list_head list;
	queue_t *queue;
} queue_ref_t;

typedef struct queue_list_entry_t
{
	char name[MAX_MQ_NAME_SIZE];
	uint32_t id;
	pid_t owner;
	spinlock_t lock;
	struct list_head queues;
} queue_list_entry_t;

void queues_init();

skiplist_t *queues_get_skl();

int syscall_mq_open(thread_t *thread, ...);

int syscall_mq_close(thread_t *thread, ...);

int syscall_mq_ctrl(thread_t *thread, ...);

#endif