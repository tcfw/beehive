#ifndef _ARCH_DEVICE_INTERRUPT_H
#define _ARCH_DEVICE_INTERRUPT_H

#include <kernel/stdint.h>

#define IRQ_TYPE_SGI (1 << 0)
#define IRQ_TYPE_PPI (1 << 1)
#define IRQ_TYPE_SPI (1 << 2)
#define IRQ_TYPE_LPI (1 << 3)

#define IRQ_BASE_NUM_SGI (0)
#define IRQ_BASE_NUM_PPI (16)
#define IRQ_BASE_NUM_SPI (32)
#define IRQ_BASE_NUM_LPI (8192)

#define IRQ_TRIGGER_EDGE (1 << 0)
#define IRQ_TRIGGER_LEVEL (1 << 1)

typedef struct device_interrupt_t
{
	uint8_t type : 4;
	uint8_t trigger : 4;
	uint32_t xrq;
} device_interrupt_t;

#endif