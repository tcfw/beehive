#include "gic.h"
#include "regs.h"
#include <kernel/arch.h>
#include <kernel/irq.h>
#include <kernel/syscall.h>
#include <kernel/panic.h>
#include <kernel/vm.h>
#include <kernel/cls.h>
#include <stdint.h>

extern unsigned long stack;

#define KSTACKEXC stack - 0x15000
#define KSTACKSWI stack - 0x05000

#define ARM4_XRQ_SYNC 0x01
#define ARM4_XRQ_IRQ 0x02
#define ARM4_XRQ_FIQ 0x03
#define ARM4_XRQ_SERROR 0x04

#define PIC_IRQ_STATUS 0x0
#define PIC_IRQ_RAWSTAT 0x1
#define PIC_IRQ_ENABLESET 0x2
#define PIC_IRQ_ENABLECLR 0x3
#define PIC_INT_SOFTSET 0x4
#define PIC_INT_SOFTCLR 0x5

#define KEXP_TOP3                                  \
	__asm__ volatile("STP x29, x30, [sp, #-16]!"); \
	__asm__ volatile("STP x18, x19, [sp, #-16]!"); \
	__asm__ volatile("STP x16, x17, [sp, #-16]!"); \
	__asm__ volatile("STP x14, x15, [sp, #-16]!"); \
	__asm__ volatile("STP x12, x13, [sp, #-16]!"); \
	__asm__ volatile("STP x10, x11, [sp, #-16]!"); \
	__asm__ volatile("STP x8, x9, [sp, #-16]!");   \
	__asm__ volatile("STP x6, x7, [sp, #-16]!");   \
	__asm__ volatile("STP x4, x5, [sp, #-16]!");   \
	__asm__ volatile("STP x2, x3, [sp, #-16]!");   \
	__asm__ volatile("STP x0, x1, [sp, #-16]!");

#define KEXP_BOT3                                \
	__asm__ volatile("LDP x0, x1, [sp], #16");   \
	__asm__ volatile("LDP x2, x3, [sp], #16");   \
	__asm__ volatile("LDP x4, x5, [sp], #16");   \
	__asm__ volatile("LDP x6, x7, [sp], #16");   \
	__asm__ volatile("LDP x8, x9, [sp], #16");   \
	__asm__ volatile("LDP x10, x11, [sp], #16"); \
	__asm__ volatile("LDP x12, x13, [sp], #16"); \
	__asm__ volatile("LDP x14, x15, [sp], #16"); \
	__asm__ volatile("LDP x16, x17, [sp], #16"); \
	__asm__ volatile("LDP x18, x19, [sp], #16"); \
	__asm__ volatile("LDP x29, x30, [sp], #16");

// handle syscall inner
void k_exphandler_swi_entry(uint64_t x0, uint64_t x1, uint64_t x2, uint64_t x3)
{
	__asm__ volatile("STR lr, [sp, #-16]!");

	// save return, switch stacks & save caller register
	__asm__ volatile("STP x29, x30, [sp, #-16]!");
	__asm__ volatile("STP x18, x19, [sp, #-16]!");
	__asm__ volatile("STP x16, x17, [sp, #-16]!");
	__asm__ volatile("STP x14, x15, [sp, #-16]!");
	__asm__ volatile("STP x12, x13, [sp, #-16]!");
	__asm__ volatile("STP x10, x11, [sp, #-16]!");
	__asm__ volatile("STP x8, x9, [sp, #-16]!");
	__asm__ volatile("STP x6, x7, [sp, #-16]!");
	__asm__ volatile("STP x4, x5, [sp, #-16]!");
	__asm__ volatile("STP x2, x3, [sp, #-16]!");
	__asm__ volatile("STR x1, [sp, #-16]!");

	// get syscall number
	uint8_t int_vector = 0;
	uint64_t esr;
	__asm__ volatile("MRS %0, ESR_EL1"
					 : "=r"(esr));
	int_vector = esr & 0xffffff;

	volatile int ret = ksyscall_entry(int_vector, x0, x1, x2, x3);
	__asm__ volatile("MOV x0, %0" ::"r"(ret)
					 : "x0");

	// restore registers
	__asm__ volatile("LDR x1, [sp], #16");
	__asm__ volatile("LDP x2, x3, [sp], #16");
	__asm__ volatile("LDP x4, x5, [sp], #16");
	__asm__ volatile("LDP x6, x7, [sp], #16");
	__asm__ volatile("LDP x8, x9, [sp], #16");
	__asm__ volatile("LDP x10, x11, [sp], #16");
	__asm__ volatile("LDP x12, x13, [sp], #16");
	__asm__ volatile("LDP x14, x15, [sp], #16");
	__asm__ volatile("LDP x16, x17, [sp], #16");
	__asm__ volatile("LDP x18, x19, [sp], #16");
	__asm__ volatile("LDP x29, x30, [sp], #16");

	// jump back to caller
	__asm__ volatile("LDR lr, [sp], #-16");
	__asm__ volatile("RET");
}

// Handle non-syscall sync exceptions
void k_exphandler_sync(uintptr_t trapFrame)
{
	thread_t *thread = get_cls()->currentThread;
	save_to_context(&thread->ctx, trapFrame);

	uint64_t esr = 0;
	__asm__ volatile("mrs %0, ESR_EL1"
					 : "=r"(esr));

	k_exphandler(ARM4_XRQ_SYNC, esr);

	// check if need to switch thread contexts
	uint64_t tpid = 0;
	__asm__ volatile("mrs %0, TPIDRRO_EL0"
					 : "=r"(tpid));
	thread = get_cls()->currentThread;

	// switch to the new thread
	if (thread->pid != tpid)
	{
		__asm__ volatile("msr TPIDRRO_EL0, %0" ::"rm"(thread->pid));
		vm_set_table(thread->vm);
		set_to_context(&thread->ctx, trapFrame);
	}
}

// Handle IRQ
void k_exphandler_irq()
{
	KEXP_TOP3;
	volatile uint64_t esr;
	__asm__ volatile("MRS %0, S3_0_c12_c12_0" // ICC_IAR1_EL1
					 : "=r"(esr));

	k_exphandler(ARM4_XRQ_IRQ, esr);

	KEXP_BOT3;
}

// Handle FIQ
void k_exphandler_fiq()
{
	k_exphandler(ARM4_XRQ_FIQ, 0);
}

// Handle serrors
void k_exphandler_serror_entry()
{
	KEXP_TOP3;
	k_exphandler(ARM4_XRQ_SERROR, 0);
	KEXP_BOT3;
}

// Acknowledge the group 1 interrupt
void ack_xrq(int xrq)
{
	gic_cpu_eoi_gp1(xrq);
}

// disable IRQ & FIQ interrupts
void disable_xrq()
{
	gic_cpu_disable();
}

// enable IRQ & FIQ interrupts && reset priority mask
void enable_xrq()
{
	gic_cpu_enable();
	gic_cpu_set_priority_mask(0xff);
}

// enable the specific interrupt number and route
// to the current PE
// all interrupts map to group 1 NS & level config
void enable_xrq_n(unsigned int xrq)
{
	uint32_t affinity = cpu_id();
	uint32_t rd = getRedistID(affinity);

	gic_redist_set_int_priority(xrq, rd, 1);
	gic_redist_set_int_group(xrq, rd, GICV3_GROUP1_NON_SECURE);
	gic_redist_enable_int(xrq, rd);
	gic_dist_enable_xrq_n(affinity, xrq);
	gic_dist_xrq_config(xrq, GICV3_CONFIG_LEVEL);
	gic_dist_target(xrq, GICV3_ROUTE_MODE_ANY, affinity);
}

// Send a software generated IRQ to all targets, except self
void send_soft_irq_all_cores(uint8_t sgi)
{
	volatile uint64_t icc_sgi = ((uint64_t)sgi & 0x04) << 24;
	icc_sgi |= (1ULL << 40); // IRM

	__asm__ volatile("MSR S3_0_c12_c11_5, %0" ::"r"(icc_sgi)); // ICC_SGI1R_EL1
}

// Send a software generated IRQ to a specific targets
void send_soft_irq(uint64_t target, uint8_t sgi)
{
	uint8_t aff3 = 0, aff2 = 0, aff1 = 0;
	uint16_t aff_target = 0;

	volatile uint64_t icc_sgi = (uint64_t)(sgi & 0x4) << 24;
	__asm__ volatile("MSR S3_0_c12_c11_5, %0" ::"r"(icc_sgi)); // ICC_SGI1R_EL1
}

// Handle FIQ exceptions
void k_fiq_exphandler(unsigned int xrq)
{
	uint64_t far = 0, par = 0, pa = 0, elr = 0;
	__asm__ volatile("MRS %0, S3_0_c6_c0_0"
					 : "=r"(far)); // FAR_EL1
	__asm__ volatile("MRS %0, S3_0_c7_c4_0"
					 : "=r"(par)); // PAR_EL1
	__asm__ volatile("MRS %0, ELR_EL1"
					 : "=r"(elr)); // ELR_EL1

	cls_t *cls = get_cls();

	switch (ESR_EXCEPTION_CLASS(xrq))
	{
	case 0b100000:
		// Instruction Abort from a lower Exception level
		terminal_logf("instruction abort from EL0 addr 0x%x", far);
		if (cls->currentThread != 0)
			terminal_logf("on PID 0x%x", cls->currentThread->pid);
		break;
	case 0b100100:
		// Data Abort from a lower exception level
		terminal_logf("data abort from EL0 addr 0x%x", far);
		if (cls->currentThread != 0)
			terminal_logf("on PID 0x%x", cls->currentThread->pid);

		break;
	case 0b100101:
		// Data abort without a change in exception level
		pa = vm_va_to_pa(vm_get_current_table(), far);

		panicf("Unhandlable Data Abort: \n\tESR: 0x%x \n\tVirtual Address: 0x%x\n\tPhysical Address: 0x%x\n\tPAR: 0x%x", xrq, far, pa, par);
		break;
	default:
		terminal_logf("unhandled FIQ 0x%x", xrq);
	}
}

// Init the vector table and GICv3 distributor and redistribute
// for the current PE
void init_xrq(void)
{
	// Load EL1 vectors
	__asm__ volatile("LDR x0, =vectors");
	__asm__ volatile("MSR VBAR_EL1, x0");

	uint32_t affinity = cpu_id();

	if (affinity == 0)
	{
		setGICAddr((void *)GIC_DIST_BASE, (void *)GIC_REDIST_BASE, (void *)GIC_CPU_BASE);
		gic_dist_enable();
	}

	uint32_t rd = getRedistID(affinity);
	gic_redist_enable(rd);

	gic_cpu_init();
}