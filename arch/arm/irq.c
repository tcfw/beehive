#include <kernel/irq.h>
#include <kernel/stdint.h>

extern unsigned long stack;

#define KSTACKEXC stack + 0x10000
#define KSTACKSWI stack + 0x05000

#define ARM4_XRQ_RESET 0x00
#define ARM4_XRQ_UNDEF 0x01
#define ARM4_XRQ_SWINT 0x02
#define ARM4_XRQ_ABRTP 0x03
#define ARM4_XRQ_ABRTD 0x04
#define ARM4_XRQ_RESV1 0x05
#define ARM4_XRQ_IRQ 0x06
#define ARM4_XRQ_FIQ 0x07

#define PIC_IRQ_STATUS 0x0
#define PIC_IRQ_RAWSTAT 0x1
#define PIC_IRQ_ENABLESET 0x2
#define PIC_IRQ_ENABLECLR 0x3
#define PIC_INT_SOFTSET 0x4
#define PIC_INT_SOFTCLR 0x5

#define KEXP_TOP3                                                \
	__asm__("mov sp, %[ps]"                                      \
			:                                                    \
			: [ps] "r"(KSTACKEXC));                              \
	__asm__("sub lr, lr, #4");                                   \
	__asm__("push {lr}");                                        \
	__asm__("push {r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12}"); \
	__asm__("mrs r0, spsr");                                     \
	__asm__("push {r0}");

#define KEXP_BOT3                                               \
	__asm__("pop {r0}");                                        \
	__asm__("msr spsr, r0");                                    \
	__asm__("pop {r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12}"); \
	__asm__("LDM sp!, {pc}^")

void __attribute__((naked)) k_exphandler_irq_entry()
{
	KEXP_TOP3;
	k_exphandler(ARM4_XRQ_IRQ);
	KEXP_BOT3;
}
void __attribute__((naked)) k_exphandler_fiq_entry()
{
	KEXP_TOP3;
	k_exphandler(ARM4_XRQ_FIQ);
	KEXP_BOT3;
}
void __attribute__((naked)) k_exphandler_reset_entry()
{
	KEXP_TOP3;
	k_exphandler(ARM4_XRQ_RESET);
	KEXP_BOT3;
}
void __attribute__((naked)) k_exphandler_undef_entry()
{
	KEXP_TOP3;
	k_exphandler(ARM4_XRQ_UNDEF);
	KEXP_BOT3;
}
void __attribute__((naked)) k_exphandler_abrtp_entry()
{
	KEXP_TOP3;
	k_exphandler(ARM4_XRQ_ABRTP);
	KEXP_BOT3;
}
void __attribute__((naked)) k_exphandler_abrtd_entry()
{
	KEXP_TOP3;
	k_exphandler(ARM4_XRQ_ABRTD);
	KEXP_BOT3;
}

void __attribute__((naked)) k_exphandler_swi_entry()
{
	// capture args
	uint32_t r0, r1, r2, r3;
	__asm__ volatile("mov r0, %0"
					 : "=r"(r0));
	__asm__ volatile("mov r1, %0"
					 : "=r"(r1));
	__asm__ volatile("mov r2, %0"
					 : "=r"(r2));
	__asm__ volatile("mov r3, %0"
					 : "=r"(r3));

	// save return, switch stacks & save caller register
	uint32_t lr;
	unsigned int usp;
	__asm__ volatile("push {lr}");
	__asm__ volatile("mov %0, sp"
					 : "=r"(usp));
	__asm__ volatile("mov sp, %[ps]"
					 :
					 : [ps] "r"(KSTACKSWI));
	__asm__ volatile("push {r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12}");
	__asm__ volatile("mrs r4, spsr");
	__asm__ volatile("push {r4}");
	__asm__ volatile("mov %[ps], lr"
					 : [ps] "=r"(lr));

	// get syscall number
	uint8_t int_vector = 0;
	int_vector = ((uint32_t *)((uintptr_t)lr - 4))[0] & 0xffff;

	int ret = ksyscall_entry(int_vector, r0, r1, r2, r3);

	// restore registers
	__asm__ volatile("pop {r4}");
	__asm__ volatile("msr spsr, r4");
	__asm__ volatile("pop {r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12}");

	// set return (if changed)
	__asm__ volatile("mov r0, %0" ::"r"(ret));

	// restore caller stack
	__asm__ volatile("mov sp, %0" ::"r"(usp));

	// jump back to caller
	__asm__ volatile("ldm sp!, {pc}^");
}

void arm4_xrqinstall(uint32_t ndx, void *addr)
{
	uint32_t *v = (uint32_t *)0x0;

	// set offset to arm __asm__ "b {addr}"
	v[ndx] = 0xEA000000 | (((uintptr_t)addr - (8 + (4 * ndx))) >> 2);
}

uint32_t arm4_cpsrget()
{
	uint32_t r;
	__asm__("mrs %[ps], cpsr"
			: [ps] "=r"(r));
	return r;
}

uint32_t arm4_spsrget()
{
	uint32_t r;

	__asm__("mrs %[ps], spsr"
			: [ps] "=r"(r));
	return r;
}

void arm4_cpsrset(uint32_t r)
{
	__asm__("msr cpsr, %[ps]"
			:
			: [ps] "r"(r));
}

void ack_all_xrq()
{
	uint32_t *picmmio;
	picmmio = (uint32_t *)0x14000000;
	picmmio[PIC_IRQ_ENABLECLR] = 0xffffffff;
}

void ack_xrq(int xrq)
{
	uint32_t *picmmio;
	picmmio = (uint32_t *)0x14000000;
	picmmio[PIC_IRQ_ENABLECLR] &= ~xrq;
}

void disable_xrq()
{
	// disable IRQ & FIQ interrupts
	arm4_cpsrset(arm4_cpsrget() & ~((1 << 6) & (1 << 7)));
}

void arm4_xrq_enable_fiq()
{
	arm4_cpsrset(arm4_cpsrget() & ~(1 << 6));
}

void arm4_xrq_enable_irq()
{
	arm4_cpsrset(arm4_cpsrget() & ~(1 << 7));
}

void enable_xrq()
{
	arm4_xrq_enable_fiq();
	arm4_xrq_enable_irq();
}

void init_xrq(void)
{
	// arm4_xrqinstall(ARM4_XRQ_RESET, &k_exphandler_reset_entry);
	arm4_xrqinstall(ARM4_XRQ_UNDEF, &k_exphandler_undef_entry);
	arm4_xrqinstall(ARM4_XRQ_SWINT, &k_exphandler_swi_entry);
	arm4_xrqinstall(ARM4_XRQ_ABRTP, &k_exphandler_abrtp_entry);
	arm4_xrqinstall(ARM4_XRQ_ABRTD, &k_exphandler_abrtd_entry);
	arm4_xrqinstall(ARM4_XRQ_IRQ, &k_exphandler_irq_entry);
	arm4_xrqinstall(ARM4_XRQ_FIQ, &k_exphandler_fiq_entry);
}