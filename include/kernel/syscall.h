#ifndef _KERNEL_SYSCALL_H
#define _KERNEL_SYSCALL_H

#include <kernel/stdint.h>
#include <kernel/thread.h>
#include <kernel/modules.h>
#include <kernel/stdarg.h>

typedef int (*syscall_handler_cb)(thread_t *thread, ...);

struct syscall_handler_t
{
	syscall_handler_cb handler;
	uint8_t argc;
};

// Register a syscall handler
void register_syscall_handler(unsigned int n, syscall_handler_cb handler, uint8_t argc);

// Handle a syscall entry
int ksyscall_entry(uint64_t type, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3);

// Initiate builtin syscalls
void syscall_init();

#define SYSCALL(n, handler, argc)                   \
	static void syscall_reg_##handler(void)         \
	{                                               \
		register_syscall_handler(n, handler, argc); \
	}                                               \
	MOD_INIT(syscall_reg_##handler);
#endif

#define DEFINE_SYSCALL0(name, n) \
	int name(thread_t *thread, ...); \
	SYSCALL(n, name, 0); \
	int name(thread_t *thread, ...) { \

#define DEFINE_SYSCALL1(name, n, arg1_type, arg1_name) \
	int name(thread_t *thread, ...); \
	SYSCALL(n, name, 1); \
	int name(thread_t *thread, ...) { \
		va_list ap; \
		va_start(ap, thread); \
		arg1_type arg1_name=va_arg(ap, arg1_type); \
		va_end(ap);

#define DEFINE_SYSCALL2(name, n, arg1_type, arg1_name, arg2_type, arg2_name) \
	int name(thread_t *thread, ...); \
	SYSCALL(n, name, 2); \
	int name(thread_t *thread, ...) { \
		va_list ap; \
		va_start(ap, thread); \
		arg1_type arg1_name=va_arg(ap, arg1_type); \
		arg2_type arg2_name=va_arg(ap, arg2_type); \
		va_end(ap);

#define DEFINE_SYSCALL3(name, n, arg1_type, arg1_name, arg2_type, arg2_name, arg3_type, arg3_name) \
	int name(thread_t *thread, ...); \
	SYSCALL(n, name, 3); \
	int name(thread_t *thread, ...) { \
		va_list ap; \
		va_start(ap, thread); \
		arg1_type arg1_name=va_arg(ap, arg1_type); \
		arg2_type arg2_name=va_arg(ap, arg2_type); \
		arg3_type arg3_name=va_arg(ap, arg3_type); \
		va_end(ap);

#define DEFINE_SYSCALL4(name, n, arg1_type, arg1_name, arg2_type, arg2_name, arg3_type, arg3_name, arg4_type, arg4_name) \
	int name(thread_t *thread, ...); \
	SYSCALL(n, name, 4); \
	int name(thread_t *thread, ...) { \
		va_list ap; \
		va_start(ap, thread); \
		arg1_type arg1_name=va_arg(ap, arg1_type); \
		arg2_type arg2_name=va_arg(ap, arg2_type); \
		arg3_type arg3_name=va_arg(ap, arg3_type); \
		arg4_type arg4_name=va_arg(ap, arg4_type); \
		va_end(ap);

#define DEFINE_SYSCALL5(name, n, arg1_type, arg1_name, arg2_type, arg2_name, arg3_type, arg3_name, arg4_type, arg4_name, arg5_type, arg5_name) \
	int name(thread_t *thread, ...); \
	SYSCALL(n, name, 5); \
	int name(thread_t *thread, ...) { \
		va_list ap; \
		va_start(ap, thread); \
		arg1_type arg1_name=va_arg(ap, arg1_type); \
		arg2_type arg2_name=va_arg(ap, arg2_type); \
		arg3_type arg3_name=va_arg(ap, arg3_type); \
		arg4_type arg4_name=va_arg(ap, arg4_type); \
		arg5_type arg5_name=va_arg(ap, arg5_type); \
		va_end(ap);

void syscall0(uint64_t syscall_no);
void syscall1(uint64_t syscall_no, uint64_t arg0);
void syscall2(uint64_t syscall_no, uint64_t arg0, uint64_t arg1);
void syscall3(uint64_t syscall_no, uint64_t arg0, uint64_t arg1, uint64_t arg2);
void syscall4(uint64_t syscall_no, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3);

#define SYSCALL_GET_PID (1)
#define SYSCALL_GET_PPID (2)
#define SYSCALL_GET_TID (3)
#define SYSCALL_PCTRL (4)
#define SYSCALL_SET_UID (5)
#define SYSCALL_SET_GID (6)
#define SYSCALL_GET_UID (7)
#define SYSCALL_GET_GID (8)
#define SYSCALL_GET_CPU (9)
#define SYSCALL_RUNNING_TIME (10)
#define SYSCALL_PROC_INFO (11)
#define SYSCALL_SCHED_GETAFFINITY (20)
#define SYSCALL_SCHED_SETAFFINITY (21)
#define SYSCALL_SCHED_GETPRIORITY (22)
#define SYSCALL_SCHED_SETPRIORITY (23)
#define SYSCALL_SCHED_YIELD (24)
#define SYSCALL_EXEC (30)
#define SYSCALL_CLONE (31)
#define SYSCALL_KNAME (40)
#define SYSCALL_SYSINFO (41)
#define SYSCALL_SET_HOSTNAME (42)
#define SYSCALL_GET_HOSTNAME (43)
#define SYSCALL_MQ_OPEN (50)
#define SYSCALL_MQ_CLOSE (51)
#define SYSCALL_MQ_CTRL (52)
#define SYSCALL_MQ_SEND (53)
#define SYSCALL_MQ_MSEND (54)
#define SYSCALL_MQ_RECV (55)
#define SYSCALL_MQ_MRECV (56)
#define SYSCALL_MQ_NOTIFY (57)
#define SYSCALL_SHM_GET (58)
#define SYSCALL_SHM_CTRL (59)
#define SYSCALL_SHM_ATTACH (60)
#define SYSCALL_SHM_DETACH (61)
#define SYSCALL_SEM_GET (62)
#define SYSCALL_SEM_OP (63)
#define SYSCALL_SEM_CTL (64)
#define SYSCALL_FUTEX (65)
#define SYSCALL_MEM_MAP (80)
#define SYSCALL_MEM_UMAP (81)
#define SYSCALL_MEM_PROTECT (82)
#define SYSCALL_BRK (83)
#define SYSCALL_GET_RANDOM (90)
#define SYSCALL_SHUTDOWN (91)
#define SYSCALL_KILL (100)
#define SYSCALL_EXIT (101)
#define SYSCALL_EXIT_GROUP (102)
#define SYSCALL_WAIT (103)
#define SYSCALL_SIG_WAIT (104)
#define SYSCALL_SIG_ACTION (105)
#define SYSCALL_SIG_PENDING (106)
#define SYSCALL_SIG_MASK (107)
#define SYSCALL_SIG_RETURN (108)
#define SYSCALL_DEV_SIGMAP (109)
#define SYSCALL_DEV_SIGACK (110)
#define SYSCALL_NANOSLEEP (120)
#define SYSCALL_TIMER_CREATE (121)
#define SYSCALL_TIMER_GET (122)
#define SYSCALL_TIMER_SET (123)
#define SYSCALL_TIMER_DELETE (124)
#define SYSCALL_TIMER_OVERRUN (125)
#define SYSCALL_CONSOLE_READ (130)
#define SYSCALL_CONSOLE_WRITE (131)
#define SYSCALL_TRACE (140)
#define SYSCALL_DEV_COUNT (150)
#define SYSCALL_DEV_INFO (151)
#define SYSCALL_DEV_CLAIM (152)
#define SYSCALL_DEV_RELEASE (153)