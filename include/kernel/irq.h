#ifndef _KERNEL_IRQ_H
#define _KERNEL_IRQ_H

#include "stdint.h"

void init_xrq(void);
void disable_xrq(void);
void enable_xrq(void);

void enable_xrq_n(unsigned int);
void ack_xrq(int);
void ack_all_xrq(void);

void k_exphandler(unsigned int type, unsigned int xrq);
int ksyscall_entry(uint64_t type, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3);

typedef void (*irq_handler_cb)(unsigned int type);

struct irq_handler_t
{
	irq_handler_cb cb;
};

void add_irq_hook(unsigned int xrq, irq_handler_cb cb);

#endif