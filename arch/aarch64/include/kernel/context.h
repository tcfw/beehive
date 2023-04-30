#ifndef _ARCH_CONTEXT_H
#define _ARCH_CONTEXT_H

struct context_t
{
	uint64_t regs[30];
	__uint128_t fpregs[30];
	uint64_t lr;
	uint64_t sp;
	uint64_t spsr;
	uint8_t ASID;
};

#endif