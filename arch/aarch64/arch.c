#include <kernel/irq.h>
#include <kernel/arch.h>
#include <kernel/tty.h>
#include <kernel/mm.h>
#include <devicetree.h>
#include "stdint.h"

uint64_t cpu_spin_table[256] = {0};

extern void secondary_boot();
extern void halt_loop();

extern uintptr_t stack;

// enable Floating point instructions
static void
enableFP()
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

uint64_t cpu_brand()
{
	uint64_t val = 0;
	__asm__ volatile("MRS %0, MIDR_EL1" ::"r"(val));
	return val;
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
	__asm__ volatile("MSR CNTP_TVAL_EL0, %0" ::"r"(value));
}

void setCounterCompareValue(uint64_t value)
{
	__asm__ volatile("MSR CNTP_CVAL_EL0, %0"
					 : "=r"(value));
}

static uint64_t psci_cpu_on(uint64_t affinity, uint64_t entrypoint)
{
	uint64_t ret = 0;

	__asm__ volatile("mov x1, %0" ::"r"(affinity));
	__asm__ volatile("mov x2, %0" ::"r"(entrypoint));
	__asm__ volatile("ldr w0, =0xc4000003");
	__asm__ volatile("mov x3, 0");
	__asm__ volatile("hvc 0");
	__asm__ volatile("mov %0, x0"
					 : "=r"(ret));

	return ret;
}

void wake_cores(void)
{
	cpu_spin_table[0] = stack;

	int cpuN = devicetree_count_dev_type("cpu");
	// int usePSCI = devicetree_count_dev_type("psci");

	for (int i = 1; i < cpuN; i++)
	{
		cpu_spin_table[i] = page_alloc_s(128 * 1024);
		int ret = psci_cpu_on(i, secondary_boot);
		if (ret < 0)
		{
			terminal_logf("failed to boot CPU: %x", ret);
		}
	}

	__asm__ volatile("sev");
}

void stop_cores(void)
{
	send_soft_irq_all_cores(0);
}

static void cpu_init(void)
{
	disable_xrq();
	init_xrq();
	enableCounter();
	setCounterValue(0);
	setCounterCompareValue(0);
	enableFP();
}

// Init arch by disabling local interrupts and initialising the global & local interrupts
void arch_init(void)
{
	cpu_init();
	k_setup_soft_irq();
}

uintptr_t ram_max(void)
{
	// TODO(tcfw): use DTB to find allocatable areas
	return RAM_MAX;
}

void wait_task(void)
{
	for (;;)
		wfi();
}