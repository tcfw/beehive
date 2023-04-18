#include <kernel/irq.h>
#include "stdint.h"

void arch_init(void)
{
	disable_xrq();
	init_xrq();
}

void arch_poweroff()
{
}

uint32_t cpu_id()
{
	uint64_t mpidr;
	uint32_t cpu_id;

	// Read the MPIDR_EL1 register into mpidr
	asm("mrs %0, mpidr_el1"
		: "=r"(mpidr));

	// Extract the Aff0 field (CPU ID) from mpidr
	cpu_id = mpidr & 0xFF;

	return cpu_id;
}

uint32_t currentEL()
{
	uint32_t cel = 0;
	__asm__ volatile("MRS %0, S3_0_C4_C2_2" ::"r"(cel)); // CurrentEL 0b11	0b000	0b0100	0b0010 0b010
	return cel >> 2;
}

void wfi()
{
	__asm__ volatile("wfi");
}