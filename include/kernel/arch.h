#include "stdint.h"

#ifndef _KERNEL_ARCH_H
#define _KERNEL_ARCH_H

void arch_init(void);
uint32_t cpu_id();
void arch_poweroff();
void wfi();

#endif