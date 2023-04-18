#include <kernel/irq.h>
#include <kernel/tty.h>
#include <kernel/strings.h>
#include <kernel/irq.h>
#include "stdint.h"

struct irq_handler_t irq_handlers[1024];

void k_exphandler(unsigned int type, unsigned int xrq)
{
	char buf[50];
	if (type == 0x2)
	{
		// ksprintf(&buf[0], "k IRQ 0x%x\n", xrq);
	}
	else
	{
		ksprintf(&buf[0], "k FIQ 0x%x\n", xrq);
		terminal_writestring(buf);
	}

	if (xrq < 1024)
	{
		struct irq_handler_t *handler = &irq_handlers[xrq];
		if (handler->cb)
		{
			handler->cb(type);
		}
	}

	ack_xrq(xrq);

	return;
}

void add_irq_hook(unsigned int xrq, irq_handler_cb cb)
{
	irq_handlers[xrq].cb = cb;
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