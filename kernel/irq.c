#include <kernel/irq.h>
#include <kernel/tty.h>
#include <kernel/strings.h>
#include "stdint.h"

void k_exphandler(unsigned int type, unsigned int xrq)
{
	char buf[50];
	ksprintf(&buf[0], "k exp %x %x\n", type, xrq);

	terminal_writestring(buf);

	return;
}

int ksyscall_entry(uint64_t type, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3)
{
	disable_xrq();

	char buf[50];
	ksprintf(&buf[0], "ksyscall type(0x%x): 0x%x 0x%x 0x%x 0x%x\n", type, arg0, arg1, arg2, arg3);

	terminal_writestring(buf);

	enable_xrq();

	return 3;
}