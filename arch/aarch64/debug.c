#include <kernel/debug.h>
#include <kernel/stdint.h>
#include <regs.h>

void enable_hw_debugger()
{
	uint64_t mdscr = 0;
	__asm__ volatile("MRS %0, MDSCR_EL1" : "=r"(mdscr));
	mdscr |= MDSCR_SOFTWARE_STEP;
	__asm__ volatile("MSR MDSCR_EL1, %0" ::"r"(mdscr));
}

void disable_hw_debugger()
{
	uint64_t mdscr = 0;
	__asm__ volatile("MRS %0, MDSCR_EL1" : "=r"(mdscr));
	mdscr &= ~MDSCR_SOFTWARE_STEP;
	__asm__ volatile("MSR MDSCR_EL1, %0" ::"r"(mdscr));
}