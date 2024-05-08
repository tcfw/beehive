#ifndef _KERNEL_THREAD_H
#define _KERNEL_THREAD_H

#include <kernel/clock.h>
#include <kernel/context.h>
#include <kernel/limits.h>
#include <kernel/list.h>
#include <kernel/sched.h>
#include <kernel/signal.h>
#include <kernel/stdint.h>
#include <kernel/sync.h>
#include <kernel/vm.h>

#define THREAD_KTHREAD (1)

#define THREAD_QUEUE_IO_WRITE (1)
#define THREAD_QUEUE_IO_READ (2)

enum Process_State
{
	RUNNING,
	STOPPED,
	ZOMBIE,
};

typedef struct vm_mapping {
	struct list_head list;
	
	uintptr_t phy_addr;
	uintptr_t vm_addr;
	size_t length;
	int flags;

	uint64_t *pg;
} vm_mapping;

typedef struct vm_t {
	vm_table *vm_table;

	struct list_head vm_maps;

	unsigned long start_code, end_code, start_data, end_data;
	unsigned long start_brk, brk, start_stack;
	unsigned long arg_start, arg_end, env_start, env_end;
} vm_t;

typedef struct process_t process_t;

typedef struct process_t
{
	process_t *parent;

	pid_t pid;

	char cmd[CMD_MAX];
	char argv[ARG_MAX];

	enum Process_State state;

	unsigned int uid;
	unsigned int euid;
	unsigned int gid;
	unsigned int egid;

	vm_t vm;

	struct list_head shm;
	struct list_head queues;

	struct list_head threads;
} process_t;

typedef struct process_list_entry_t
{
	struct list_head list;
	process_t *process;
} process_list_entry_t;

enum Thread_State
{
	THREAD_RUNNING,
	THREAD_SLEEPING,
	THREAD_UNINT_SLEEPING,
	THREAD_STOPPED,
};

enum Wait_Cond_Type
{
	SLEEP,
	QUEUE_IO,
	WAIT,
};

typedef struct thread_wait_cond
{
	enum Wait_Cond_Type type;
} thread_wait_cond;

struct thread_wait_cond_sleep
{
	thread_wait_cond cond;
	timespec_t timer;
	timespec_t *user_rem;
};

struct thread_wait_cond_queue_io
{
	thread_wait_cond cond;
	uint64_t flags;
	struct list_head queues;
	void *buf;
};

typedef struct thread_timing_t
{
	uint64_t total_execution;
	uint64_t total_system;
	uint64_t total_user;
	uint64_t total_wait;

	uint64_t last_system;
	uint64_t last_user;
	uint64_t last_wait;
} thread_timing_t;

typedef struct sched_class_t sched_class_t;
typedef struct sched_entity_t sched_entity_t;

typedef struct thread_t
{
	process_t *process;
	tid_t tid;

	context_t ctx;
	uint64_t flags;
	enum Thread_State state;
	uint64_t affinity;

	thread_timing_t timing;
	sched_class_t *sched_class;
	sched_entity_t sched_entity;

	thread_sigactions_t *sigactions;

	thread_wait_cond *wc;
} thread_t;

typedef struct thread_list_entry_t
{
	struct list_head list;
	thread_t *thread;
} thread_list_entry_t;

typedef struct thread_table
{
	thread_list_entry_t list;
	spinlock_t lock;
} thread_table;

void init_kthread_proc();

// Init a thread
void init_thread(thread_t *thread);

// Init a thread context
void init_context(context_t *ctx);

// Update the thread context to be runnable in a kernel exception level
void kthread_context(context_t *ctx, void *data);

void arch_thread_prep_switch(thread_t *thread);

// create a kernel thread
thread_t *create_kthread(void(entry)(void *), const char *name, void *data);

// Mark a thread as awake
void wake_thread(thread_t *thread);

void sleep_kthread(const timespec_t *ts, timespec_t *rem);

// Put thread to sleep for ts time
void sleep_thread(thread_t *thread, const timespec_t *ts, timespec_t *user_rem);

// Put a thread to sleep to wait for cond
void thread_wait_for_cond(thread_t *thread, const thread_wait_cond *cond);

// Populate data for return from wait cond
void thread_return_wc(thread_t *thread, void *data1);

int can_wake_thread(thread_t *thread);

thread_t *get_thread_by_pid(pid_t pid);

void free_thread(thread_t *thread);

#endif