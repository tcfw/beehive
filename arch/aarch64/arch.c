#include <kernel/irq.h>
#include <kernel/arch.h>
#include "stdint.h"

// enable Floating point instructions
static void enableFP()
{
	volatile uint64_t cpacrel1 = 0;
	__asm__ volatile("MRS %0, CPACR_EL1" ::"r"(cpacrel1));

	cpacrel1 |= (1 << 20) | (1 << 21);

	__asm__ volatile("MSR CPACR_EL1, %0"
					 : "=r"(cpacrel1));
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

uint64_t getCounterValue()
{
	uint64_t value = 0;
	__asm__ volatile("MRS %0, CNTP_TVAL_EL0" ::"r"(value));

	return value & 0xffffffff;
}

void enableCounter()
{
	uint64_t cnt_ctl = 0x1;
	__asm__ volatile("MSR CNTP_CTL_EL0, %0"
					 : "=r"(cnt_ctl));
}

uint64_t getSysCounterValue()
{
	uint64_t value = 0;
	__asm__ volatile("MRS %0, CNTPCT_EL0" ::"r"(value));
	return value;
}

uint64_t getCounterFreq()
{
	uint64_t freq = 0;
	__asm__ volatile("MRS %0, CNTFRQ_EL0" ::"r"(freq));
	return (freq & 0xffffffff);
}

void setCounterValue(uint64_t value)
{
	// armv8-a only supports a 32bit counter
	value &= 0xffffffff;
	__asm__ volatile("MSR CNTP_TVAL_EL0, %0"
					 : "=r"(value));
}

void setCounterCompareValue(uint64_t value)
{
	__asm__ volatile("MSR CNTP_CVAL_EL0, %0"
					 : "=r"(value));
}

// Init arch by disabling local interrupts and initialising the global & local interrupts
void arch_init(void)
{
	disable_xrq();
	init_xrq();
	enableCounter();
	setCounterValue(0);
	setCounterCompareValue(0);
	enableFP();
}
