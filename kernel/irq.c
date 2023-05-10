#include <kernel/irq.h>
#include <kernel/tty.h>
#include <kernel/strings.h>
#include <kernel/irq.h>
#include "stdint.h"

#define IRQ_HANDLER_MAX 1024

struct irq_handler_t irq_handlers[IRQ_HANDLER_MAX];

// FIQ & IRQ handler
void k_exphandler(unsigned int type, unsigned int xrq)
{
	if (type != 0x2)
	{
		terminal_logf("k FIQ 0x%x\n", xrq);
	}
	else if (xrq < IRQ_HANDLER_MAX)
	{
		struct irq_handler_t *handler = &irq_handlers[xrq];
		if (handler->khandler)
		{
			handler->khandler(type);
		}
		else if (handler->pid)
		{
			// enqueue signal to proc
		}
	}

	ack_xrq(xrq);

	return;
}

// Assign an interrupt to a callback
void assign_irq_hook(unsigned int xrq, irq_handler_cb cb)
{
	irq_handlers[xrq].khandler = cb;
	enable_xrq_n(xrq);
}
