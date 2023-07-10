#ifndef _ARCH_CONTEXT_H
#define _ARCH_CONTEXT_H

#include "stdint.h"

typedef struct context_t
{
	uint64_t regs[29];
	__uint128_t fpregs[32];
	uint64_t pc;
	uint64_t sp;
	uint64_t spsr;
	uint16_t asid;
} context_t;

#endif