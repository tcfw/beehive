#include <kernel/irq.h>
#include <kernel/tty.h>
#include <kernel/strings.h>
#include <kernel/irq.h>
#include <kernel/arch.h>
#include <kernel/clock.h>
#include "stdint.h"

extern void halt_loop();
static void halt_core(__attribute__((unused)) unsigned int _);
static void clock_tick(unsigned int type);

#define IRQ_HANDLER_MAX (1024)

struct irq_handler_t irq_handlers[IRQ_HANDLER_MAX];

// FIQ & IRQ handler
void k_exphandler(unsigned int type, unsigned int xrq)
{
	if (type != 0x2)
	{
		k_fiq_exphandler(xrq);
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

// setup built in software IRQs
void k_setup_soft_irq()
{
	assign_irq_hook(0, halt_core);
	enable_xrq_n(0);
}

void k_setup_clock_irq()
{
	// EL1 physical timer
	assign_irq_hook(30, clock_tick);
	enable_xrq_n(30);

	// EL1 virtual timer
	assign_irq_hook(27, clock_tick);
	enable_xrq_n(27);
}

static void clock_tick(unsigned int type)
{
	terminal_logf("TICK! on 0x%x", cpu_id());
	struct clocksource_t *cs = clock_first(CS_GLOBAL);
	cs->disableIRQ(cs, 0);
}

static void halt_core(__attribute__((unused)) unsigned int _)
{
	terminal_logf("Core Halting 0x%x", cpu_id());
	halt_loop();
}