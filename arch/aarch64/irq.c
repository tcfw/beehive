#include "gic.h"
#include "regs.h"
#include "errno.h"
#include <kernel/arch.h>
#include <kernel/cls.h>
#include <kernel/irq.h>
#include <kernel/panic.h>
#include <kernel/syscall.h>
#include <kernel/tty.h>
#include <kernel/vm.h>
#include <kernel/thread.h>
#include <kernel/cls.h>
#include <kernel/stdint.h>
#include "regs.h"

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

#define KEXP_TOP3                                             \
	uint64_t spsr = 0;                                        \
	__asm__ volatile("mrs %0, SPSR_EL1"                       \
					 : "=r"(spsr));                           \
	thread_t *thread = get_cls()->rq.current_thread;          \
	int didsave = 0;                                          \
	int iskthread = 0;                                        \
	if (thread != 0)                                          \
		iskthread = thread->flags & THREAD_KTHREAD;           \
	if ((spsr & SPSR_M_MASK) == SPSR_M_EL0 || iskthread != 0) \
	{                                                         \
		didsave = 1;                                          \
		save_to_context(&thread->ctx, trapFrame);             \
	}

#define KEXP_BOT3                                    \
	thread_t *next = get_cls()->rq.current_thread;   \
	if (next != thread && didsave)                   \
	{                                                \
		thread = next;                               \
		vm_set_table(thread->process->vm.vm_table, thread->process->pid); \
		set_to_context(&thread->ctx, trapFrame);     \
	}

// handle syscall inner
void k_exphandler_swi_entry(uintptr_t trapFrame)
{
	KEXP_TOP3

	// check svc number
	uint8_t int_vector = 0;
	__asm__ volatile("MRS %0, ESR_EL1"
					 : "=r"(int_vector));
	int_vector &= 0xffffff;

	if (int_vector != 0)
	{
		int ret = -ERRNOSYS;
		__asm__ volatile("MOV x0, %0" ::"r"(ret)
						 : "x0");
		return;
	}

	uint64_t x0, x1, x2, x3, x4;
	x0 = thread->ctx.regs[0];
	x1 = thread->ctx.regs[1];
	x2 = thread->ctx.regs[2];
	x3 = thread->ctx.regs[3];
	x4 = thread->ctx.regs[4];

	volatile int ret = ksyscall_entry(x0, x1, x2, x3, x4);
	thread->ctx.regs[0] = ret;

	uint64_t *pending_irq = &get_cls()->pending_irq;
	if (*pending_irq)
	{
		k_deferred_exphandler(ARM4_XRQ_IRQ, *pending_irq);
		pending_irq = 0;
	}

	// clear ESR
	__asm__ volatile("MOV x0, #0; MSR ESR_EL1, x0");

	KEXP_BOT3
}

// Handle non-syscall sync exceptions
void k_exphandler_sync(uintptr_t trapFrame)
{
	KEXP_TOP3;
	disable_irq();

	uint64_t esr = 0;
	__asm__ volatile("mrs %0, ESR_EL1"
					 : "=r"(esr));

	k_exphandler(ARM4_XRQ_SYNC, esr, 0);

	KEXP_BOT3;
}

// Handle IRQ
void k_exphandler_irq(uintptr_t trapFrame)
{
	KEXP_TOP3;

	disable_irq();

	volatile uint64_t xrq;
	__asm__ volatile("MRS %0, S3_0_c12_c12_0" // ICC_IAR1_EL1
					 : "=r"(xrq));

	volatile uint64_t esr;
	__asm__ volatile("MRS %0, ESR_EL1"
					 : "=r"(esr));

	if (ESR_EXCEPTION_CLASS(esr) == ESR_EXCEPTION_SVC)
	{
		// defer IRQ until syscall is complete
		get_cls()->pending_irq |= (1 << xrq);
	}
	else
		k_exphandler(ARM4_XRQ_IRQ, xrq, 0);

	KEXP_BOT3;
}

// Handle FIQ
void k_exphandler_fiq()
{
	k_exphandler(ARM4_XRQ_FIQ, 0, 0);
}

// Handle serrors
void k_exphandler_serror_entry(uintptr_t trapFrame)
{
	KEXP_TOP3;
	k_exphandler(ARM4_XRQ_SERROR, 0, 0);
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

// Disable interrutps for the local core
// for when handling syscalls
void disable_irq(void)
{
	// Disable all DAIF
	__asm__ volatile("MOV x0, #0x3C0; MSR DAIF, X0; ISB");
}

// Enable interrupts for the local core
// for when handling syscalls
void enable_irq(void)
{
	// Enable all DAIF
	__asm__ volatile("ISB; MOV x0, #0; MSR DAIF, X0; ISB");
}

// enable the specific interrupt number and route
// to the current PE
// all interrupts map to group 1 NS & level config
void enable_xrq_n(unsigned int xrq)
{
	uint32_t affinity = cpu_id();
	uint32_t rd = getRedistID(affinity);

	gic_redist_set_int_priority(xrq, rd, 0);
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
	case ESR_EXCEPTION_INSTRUCTION_ABORT_LOWER_EL:
		terminal_logf("instruction abort from EL0 addr 0x%X", far);
		if (cls->rq.current_thread != 0)
			terminal_logf("on PID 0x%x", cls->rq.current_thread->process->pid);
		// send SIGILL
		break;
	case ESR_EXCEPTION_INSTRUCTION_ABORT_SAME_EL:
		if (cls->cfe != EXCEPTION_UNKNOWN)
		{
			if (cls->cfe_handle != 0)
				cls->cfe_handle();
			// send SIGSYS?
		}
		else
			panicf("Instruction abort at 0x%x", far);
	case ESR_EXCEPTION_DATA_ABORT_LOWER_EL:
		terminal_logf("data abort from EL0 addr 0x%x", far);
		if (cls->rq.current_thread != 0)
			terminal_logf("on PID 0x%x", cls->rq.current_thread->process->pid);
		// send SIGSEGV

		break;
	case ESR_EXCEPTION_DATA_ABORT_SAME_EL:
		if (cls->cfe != EXCEPTION_UNKNOWN)
		{
			if (cls->cfe_handle != 0)
				cls->cfe_handle();

			// send SIGSEGV
		}
		else
		{
			pa = vm_va_to_pa(vm_get_current_table(), far);
			panicf("Unhandlable Data Abort: \n\tELR: 0x%X \n\tESR: 0x%x \n\tVirtual Address: 0x%X\n\tPhysical Address: 0x%X\n\tPAR: 0x%X", elr, xrq, far, pa, par);
		}
		break;
	default:
		panicf("unhandled FIQ(0x%x) FAR: 0x%X", xrq, far);
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