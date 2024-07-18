#include <kernel/cls.h>
#include <kernel/arch.h>
#include <kernel/clock.h>
#include <kernel/irq.h>
#include <kernel/sched.h>
#include <kernel/stdint.h>
#include <kernel/strings.h>
#include <kernel/tty.h>
#include <kernel/wait.h>

extern void halt_loop();
static void halt_core(__attribute__((unused)) unsigned int _);
static void clock_tick(unsigned int type);

#define IRQ_HANDLER_MAX (1024)

struct irq_handler_t irq_handlers[IRQ_HANDLER_MAX];

// FIQ & IRQ handler
void k_exphandler(unsigned int type, unsigned int xrq, int deferred)
{
	switch (type)
	{
	case 0x1: // software
		k_sync_exphandler(xrq);
		goto ack_sched;
	case 0x3: // FIQ
		k_fiq_exphandler(xrq);
		return;
	case 0x4: // System Error
		break;
	}

	if (xrq < IRQ_HANDLER_MAX)
	{
		struct irq_handler_t *handler = &irq_handlers[xrq];
		if (handler->khandler)
		{
			handler->khandler(xrq);
			ack_xrq(xrq);
		}
		else if (handler->pid)
		{
			// enqueue signal to proc
		}
		else
			terminal_logf("unhandled IRQ 0x%X", xrq);
	}

ack_sched:
	if (current->state != THREAD_RUNNING)
		schedule();

	return;
}

void k_deferred_exphandler(unsigned int type, uint64_t xrq_map)
{
	while (xrq_map != 0)
	{
		uint64_t fsb = xrq_map & -xrq_map;
		xrq_map &= ~fsb;

		uint64_t xrq = 0;
		while (fsb != 0)
		{
			xrq++;
			fsb >>= 1;
		}

		k_exphandler(type, xrq - 1, 1);
	}
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
	assign_irq_hook(SOFT_IRQ_HALT_CORE, halt_core);
	assign_irq_hook(SOFT_IRQ_THREAD_STOP, thread_stop_core);
	enable_xrq_n(0);
	enable_xrq_n(1);
}

void k_setup_clock_irq()
{
	// EL1 physical timer
	assign_irq_hook(30, clock_tick);
	enable_xrq_n_prio(30, 1);
	xrq_set_trigger_type(30, 1);

	// EL1 virtual timer
	assign_irq_hook(27, clock_tick);
	enable_xrq_n_prio(27, 1);
	xrq_set_trigger_type(27, 1);
}

static void clock_tick(unsigned int xrq)
{
	struct clocksource_t *cs = clock_first(CS_GLOBAL);
	cs->disable(cs);
	cs->disableIRQ(cs);

	schedule();

	uint64_t freq = cs->getFreq(cs);
	cs->countNTicks(cs, freq / 250);
	cs->enableIRQ(cs);
	cs->enable(cs);
}

static void halt_core(__attribute__((unused)) unsigned int _)
{
	terminal_logf("Core Halting 0x%x", cpu_id());
	halt_loop();
}