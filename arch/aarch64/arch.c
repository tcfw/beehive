#include <kernel/irq.h>
#include "stdint.h"

// Init arch by disabling local interrupts and initialising the global & local interrupts
void arch_init(void)
{
	disable_xrq();
	init_xrq();
}

// Power down
void arch_poweroff()
{
}

// Get the current PE ID
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

// Get the current EL state
uint32_t currentEL()
{
	uint32_t cel = 0;
	__asm__ volatile("MRS %0, S3_0_C4_C2_2" ::"r"(cel)); // CurrentEL
	return cel >> 2;
}

// Wait for an interrupt
void wfi()
{
	__asm__ volatile("wfi");
}