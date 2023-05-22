#ifndef _KERNEL_ARCH_H
#define _KERNEL_ARCH_H

#include "stdint.h"

void arch_init(void);
void arch_poweroff();

uint32_t cpu_id();
uint64_t cpu_brand();
void wfi();

void enableCounter();
uint64_t getCounterFreq();
void setCounterValue(uint64_t);
uint64_t getCounterValue();
uint64_t getSysCounterValue();
void setCounterCompareValue(uint64_t);

void wake_cores(void);

#endif