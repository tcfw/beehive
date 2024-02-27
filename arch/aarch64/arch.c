#include "clocks.h"
#include <kernel/devicetree.h>
#include <kernel/stdint.h>
#include <kernel/devicetree.h>
#include <kernel/arch.h>
#include <kernel/clock.h>
#include <kernel/irq.h>
#include <kernel/mm.h>
#include <kernel/tty.h>
#include <kernel/strings.h>
#include <kernel/panic.h>
#include <kernel/vm.h>

uintptr_t cpu_spin_table[256] = {0};

extern void secondary_boot();
extern void _start();
extern void halt_loop();

extern uintptr_t stack;

#define CORE_BOOT_SP_SIZE (128 * 1024)

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
	__asm__ volatile("ldr w0, =0x84000008");
	__asm__ volatile("hvc 0");
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

static uint64_t psci_cpu_on(uint64_t affinity, uint64_t entrypoint)
{
	uint64_t ret = 0;

	__asm__ volatile("mov x1, %0" ::"r"(affinity));
	__asm__ volatile("mov x2, %0" ::"r"(entrypoint));
	__asm__ volatile("ldr x0, =0xc4000003");
	__asm__ volatile("mov x3, 0");
	__asm__ volatile("hvc 0");
	__asm__ volatile("mov %0, x0"
					 : "=r"(ret));

	return ret;
}

void wake_cores(void)
{
	cpu_spin_table[0] = (uintptr_t)&stack;

	int cpuN = devicetree_count_dev_type("cpu");
	// int usePSCI = devicetree_count_dev_type("psci");

	for (int i = 1; i < cpuN; i++)
	{
		cpu_spin_table[i] = (uintptr_t)page_alloc_s(CORE_BOOT_SP_SIZE) + CORE_BOOT_SP_SIZE;
		int ret = psci_cpu_on(i, (uint64_t)_start);
		if (ret < 0)
			terminal_logf("failed to boot CPU: %x", ret);
	}

	__asm__ volatile("sev");
}

void stop_cores(void)
{
	send_soft_irq_all_cores(0);
}

void core_init(void)
{
	disable_xrq();
	init_xrq();
	enableFP();
}

// Init arch by disabling local interrupts and initialising the global & local interrupts
void arch_init(void)
{
	k_setup_soft_irq();
	k_setup_clock_irq();
}

uintptr_t ram_max(void)
{
	return ram_start() + ram_size();
}

uintptr_t ram_start(void) {
	void *mem_map = devicetree_find_node("/memory");
	if (mem_map == 0)
		panic("could not find memory node in device tree");

	if (strcmp(devicetree_get_property(mem_map, "device_type"), "memory") != 0)
		panic("memory node has unexpected device_type propert");

	uint32_t *regs = (uint32_t *)devicetree_get_property(mem_map, "reg");
	uint32_t *addrCells = (uint32_t *)devicetree_get_root_property("#address-cells");

	uint64_t start = 0;

	if (BIG_ENDIAN_UINT32(*addrCells) == 2)
	{
		start = BIG_ENDIAN_UINT64(*(uint64_t *)regs);
	}
	else if (BIG_ENDIAN_UINT32(*addrCells) == 1)
	{
		start = (uint32_t)BIG_ENDIAN_UINT32(*(uint32_t *)regs);
	}
	else
		panic("unsupported #address-cells");

	return start;
}

uintptr_t ram_size(void)
{
	void *mem_map = devicetree_find_node("/memory");
	if (mem_map == 0)
		panic("could not find memory node in device tree");

	if (strcmp(devicetree_get_property(mem_map, "device_type"), "memory") != 0)
		panic("memory node has unexpected device_type propert");

	uint32_t *regs = (uint32_t *)devicetree_get_property(mem_map, "reg");
	uint32_t *addrCells = (uint32_t *)devicetree_get_root_property("#address-cells");
	uint32_t *sizeCells = (uint32_t *)devicetree_get_root_property("#size-cells");

	uint64_t size = 0;

	if (BIG_ENDIAN_UINT32(*addrCells) == 2)
		regs += 2;
	else if (BIG_ENDIAN_UINT32(*addrCells) == 1)
		regs++;
	else
		panic("unsupported #address-cells");

	if (BIG_ENDIAN_UINT32(*sizeCells) == 2)
		size = BIG_ENDIAN_UINT64(*(uint64_t *)regs);
	else if (BIG_ENDIAN_UINT32(*sizeCells) == 1)
		size = BIG_ENDIAN_UINT32(*regs);
	else
		panic("unsupported #size-cells");

	return size;
}

void wait_task(void)
{
	for (;;)
		wfi();
}