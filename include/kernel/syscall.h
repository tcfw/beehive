#ifndef _KERNEL_SYSCALL_H
#define _KERNEL_SYSCALL_H

#include <kernel/stdint.h>
#include <kernel/thread.h>
#include <kernel/modules.h>
#include <kernel/stdarg.h>
#include <syscall_num.h>

typedef uint64_t (*syscall_handler_cb)(thread_t *thread, ...);

struct syscall_handler_t
{
	syscall_handler_cb handler;
	uint8_t argc;
};

// Register a syscall handler
void register_syscall_handler(unsigned int n, syscall_handler_cb handler, uint8_t argc);

// Handle a syscall entry
uint64_t ksyscall_entry(uint64_t type, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4);

// Initiate builtin syscalls
void syscall_init();

#define SYSCALL(n, handler, argc)                   \
	static void syscall_reg_##handler(void)         \
	{                                               \
		register_syscall_handler(n, handler, argc); \
	}                                               \
	MOD_INIT(syscall_reg_##handler);
#endif

#define DEFINE_SYSCALL0(name, n)          \
	uint64_t name(thread_t *thread, ...); \
	SYSCALL(n, name, 0);                  \
	uint64_t name(thread_t *thread, ...)

#define DEFINE_SYSCALL1(name, n, arg1_type, arg1_name)          \
	uint64_t _do_##name(thread_t *thread, arg1_type arg1_name); \
	uint64_t name(thread_t *thread, ...)                        \
	{                                                           \
		va_list ap;                                             \
		va_start(ap, thread);                                   \
		arg1_type arg1_name = va_arg(ap, arg1_type);            \
		va_end(ap);                                             \
		return _do_##name(thread, arg1_name);                   \
	};                                                          \
	SYSCALL(n, name, 1);                                        \
	uint64_t _do_##name(thread_t *thread, arg1_type arg1_name)

#define DEFINE_SYSCALL2(name, n, arg1_type, arg1_name, arg2_type, arg2_name)         \
	uint64_t _do_##name(thread_t *thread, arg1_type arg1_name, arg2_type arg2_name); \
	uint64_t name(thread_t *thread, ...)                                             \
	{                                                                                \
		va_list ap;                                                                  \
		va_start(ap, thread);                                                        \
		arg1_type arg1_name = va_arg(ap, arg1_type);                                 \
		arg2_type arg2_name = va_arg(ap, arg2_type);                                 \
		va_end(ap);                                                                  \
		return _do_##name(thread, arg1_name, arg2_name);                             \
	}                                                                                \
	SYSCALL(n, name, 2);                                                             \
	uint64_t _do_##name(thread_t *thread, arg1_type arg1_name, arg2_type arg2_name)

#define DEFINE_SYSCALL3(name, n, arg1_type, arg1_name, arg2_type, arg2_name, arg3_type, arg3_name)        \
	uint64_t _do_##name(thread_t *thread, arg1_type arg1_name, arg2_type arg2_name, arg3_type arg3_name); \
	uint64_t name(thread_t *thread, ...)                                                                  \
	{                                                                                                     \
		va_list ap;                                                                                       \
		va_start(ap, thread);                                                                             \
		arg1_type arg1_name = va_arg(ap, arg1_type);                                                      \
		arg2_type arg2_name = va_arg(ap, arg2_type);                                                      \
		arg3_type arg3_name = va_arg(ap, arg3_type);                                                      \
		va_end(ap);                                                                                       \
		return _do_##name(thread, arg1_name, arg2_name, arg3_name);                                       \
	}                                                                                                     \
	SYSCALL(n, name, 3);                                                                                  \
	uint64_t _do_##name(thread_t *thread, arg1_type arg1_name, arg2_type arg2_name, arg3_type arg3_name)

#define DEFINE_SYSCALL4(name, n, arg1_type, arg1_name, arg2_type, arg2_name, arg3_type, arg3_name, arg4_type, arg4_name)       \
	uint64_t _do_##name(thread_t *thread, arg1_type arg1_name, arg2_type arg2_name, arg3_type arg3_name, arg4_type arg4_name); \
	uint64_t name(thread_t *thread, ...)                                                                                       \
	{                                                                                                                          \
		va_list ap;                                                                                                            \
		va_start(ap, thread);                                                                                                  \
		arg1_type arg1_name = va_arg(ap, arg1_type);                                                                           \
		arg2_type arg2_name = va_arg(ap, arg2_type);                                                                           \
		arg3_type arg3_name = va_arg(ap, arg3_type);                                                                           \
		arg4_type arg4_name = va_arg(ap, arg4_type);                                                                           \
		va_end(ap);                                                                                                            \
		return _do_##name(thread, arg1_name, arg2_name, arg3_name, arg4_name);                                                 \
	}                                                                                                                          \
	SYSCALL(n, name, 4);                                                                                                       \
	uint64_t _do_##name(thread_t *thread, arg1_type arg1_name, arg2_type arg2_name, arg3_type arg3_name, arg4_type arg4_name)

#define DEFINE_SYSCALL5(name, n, arg1_type, arg1_name, arg2_type, arg2_name, arg3_type, arg3_name, arg4_type, arg4_name, arg5_type, arg5_name)      \
	uint64_t _do_##name(thread_t *thread, arg1_type arg1_name, arg2_type arg2_name, arg3_type arg3_name, arg4_type arg4_name, arg5_type arg5_name); \
	uint64_t name(thread_t *thread, ...)                                                                                                            \
	{                                                                                                                                               \
		va_list ap;                                                                                                                                 \
		va_start(ap, thread);                                                                                                                       \
		arg1_type arg1_name = va_arg(ap, arg1_type);                                                                                                \
		arg2_type arg2_name = va_arg(ap, arg2_type);                                                                                                \
		arg3_type arg3_name = va_arg(ap, arg3_type);                                                                                                \
		arg4_type arg4_name = va_arg(ap, arg4_type);                                                                                                \
		arg5_type arg5_name = va_arg(ap, arg5_type);                                                                                                \
		va_end(ap);                                                                                                                                 \
		return _do_##name(thread, arg1_name, arg2_name, arg3_name, arg4_name, arg5_name);                                                           \
	}                                                                                                                                               \
	SYSCALL(n, name, 5);                                                                                                                            \
	uint64_t _do_##name(thread_t *thread, arg1_type arg1_name, arg2_type arg2_name, arg3_type arg3_name, arg4_type arg4_name, arg5_type arg5_name)

uint64_t syscall0(uint64_t syscall_no);
uint64_t syscall1(uint64_t syscall_no, uint64_t arg0);
uint64_t syscall2(uint64_t syscall_no, uint64_t arg0, uint64_t arg1);
uint64_t syscall3(uint64_t syscall_no, uint64_t arg0, uint64_t arg1, uint64_t arg2);
uint64_t syscall4(uint64_t syscall_no, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3);
