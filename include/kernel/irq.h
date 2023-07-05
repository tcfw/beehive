#ifndef _KERNEL_IRQ_H
#define _KERNEL_IRQ_H

#include "unistd.h"
#include "stdint.h"
#include <kernel/thread.h>

#define IRQ_FIQ (0x2)

void init_xrq(void);
void disable_xrq(void);
void enable_xrq(void);

void enable_xrq_n(unsigned int);
void ack_xrq(int);
void ack_all_xrq(void);

void k_exphandler(unsigned int type, unsigned int xrq);
void k_fiq_exphandler(unsigned int xrq);
void k_setup_soft_irq();

typedef void (*irq_handler_cb)(unsigned int type);

struct irq_handler_t
{
	irq_handler_cb khandler;
	pid_t pid;
};

void assign_irq_hook(unsigned int xrq, irq_handler_cb cb);

void send_soft_irq_all_cores(uint8_t sgi);
void send_soft_irq(uint64_t target, uint8_t sgi);

#endif