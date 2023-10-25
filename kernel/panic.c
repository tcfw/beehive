#include <kernel/panic.h>
#include <kernel/strings.h>
#include <kernel/tty.h>
#include <kernel/arch.h>
#include <kernel/irq.h>

static void _panic();

void panic(char *msg)
{
	terminal_log(msg);
	_panic();
}

void panicf(char *fmt, ...)
{
	char buf[2048];

	__builtin_va_list argp;
	__builtin_va_start(argp, fmt);

	ksprintfz(buf, fmt, argp);

	__builtin_va_end(argp);

	terminal_log("PANIC! ");
	terminal_log(buf);
	_panic();
}

static void _panic()
{
	stop_cores();
	k_exphandler(0x2, 0, 0);
}