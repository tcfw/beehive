#include <kernel/context.h>
#include <kernel/arch.h>
#include "regs.h"

void init_context(context_t *ctx)
{
	// set sane values for SPSR
	ctx->spsr = (SPSR_UAO | SPSR_PAN | SPSR_IL |
				 SPSR_D | SPSR_A | SPSR_I | SPSR_F |
				 SPSR_M_EL0);
	ctx->sp = 0;
	ctx->pc = 0;
}

void switch_to_context(context_t *ctx)
{
	// setup pc, sp & pstate
	__asm__ volatile("msr ELR_EL1, %0" ::"r"(ctx->pc));
	__asm__ volatile("msr SP_EL0, %0" ::"r"(ctx->sp));
	__asm__ volatile("msr SPSR_EL1, %0" ::"r"(ctx->spsr));

	__asm__ volatile("fmov d0, %0" ::"r"(ctx->fpregs[0]));
	__asm__ volatile("fmov d1, %0" ::"r"(ctx->fpregs[1]));
	__asm__ volatile("fmov d2, %0" ::"r"(ctx->fpregs[2]));
	__asm__ volatile("fmov d3, %0" ::"r"(ctx->fpregs[3]));
	__asm__ volatile("fmov d4, %0" ::"r"(ctx->fpregs[4]));
	__asm__ volatile("fmov d5, %0" ::"r"(ctx->fpregs[5]));
	__asm__ volatile("fmov d6, %0" ::"r"(ctx->fpregs[6]));
	__asm__ volatile("fmov d7, %0" ::"r"(ctx->fpregs[7]));
	__asm__ volatile("fmov d8, %0" ::"r"(ctx->fpregs[8]));
	__asm__ volatile("fmov d9, %0" ::"r"(ctx->fpregs[9]));
	__asm__ volatile("fmov d10, %0" ::"r"(ctx->fpregs[10]));
	__asm__ volatile("fmov d11, %0" ::"r"(ctx->fpregs[11]));
	__asm__ volatile("fmov d12, %0" ::"r"(ctx->fpregs[12]));
	__asm__ volatile("fmov d13, %0" ::"r"(ctx->fpregs[13]));
	__asm__ volatile("fmov d14, %0" ::"r"(ctx->fpregs[14]));
	__asm__ volatile("fmov d15, %0" ::"r"(ctx->fpregs[15]));
	__asm__ volatile("fmov d16, %0" ::"r"(ctx->fpregs[16]));
	__asm__ volatile("fmov d17, %0" ::"r"(ctx->fpregs[17]));
	__asm__ volatile("fmov d18, %0" ::"r"(ctx->fpregs[18]));
	__asm__ volatile("fmov d19, %0" ::"r"(ctx->fpregs[19]));
	__asm__ volatile("fmov d20, %0" ::"r"(ctx->fpregs[20]));
	__asm__ volatile("fmov d21, %0" ::"r"(ctx->fpregs[21]));
	__asm__ volatile("fmov d22, %0" ::"r"(ctx->fpregs[22]));
	__asm__ volatile("fmov d23, %0" ::"r"(ctx->fpregs[23]));
	__asm__ volatile("fmov d24, %0" ::"r"(ctx->fpregs[24]));
	__asm__ volatile("fmov d25, %0" ::"r"(ctx->fpregs[25]));
	__asm__ volatile("fmov d26, %0" ::"r"(ctx->fpregs[26]));
	__asm__ volatile("fmov d27, %0" ::"r"(ctx->fpregs[27]));
	__asm__ volatile("fmov d28, %0" ::"r"(ctx->fpregs[28]));
	__asm__ volatile("fmov d29, %0" ::"r"(ctx->fpregs[29]));
	__asm__ volatile("fmov d30, %0" ::"r"(ctx->fpregs[30]));
	__asm__ volatile("fmov d31, %0" ::"r"(ctx->fpregs[31]));

	__asm__ volatile("mov x2, %0" ::"r"(ctx->regs[2]));
	__asm__ volatile("mov x3, %0" ::"r"(ctx->regs[3]));
	__asm__ volatile("mov x4, %0" ::"r"(ctx->regs[4]));
	__asm__ volatile("mov x5, %0" ::"r"(ctx->regs[5]));
	__asm__ volatile("mov x6, %0" ::"r"(ctx->regs[6]));
	__asm__ volatile("mov x7, %0" ::"r"(ctx->regs[7]));
	__asm__ volatile("mov x8, %0" ::"r"(ctx->regs[8]));
	__asm__ volatile("mov x9, %0" ::"r"(ctx->regs[9]));
	__asm__ volatile("mov x10, %0" ::"r"(ctx->regs[10]));
	__asm__ volatile("mov x11, %0" ::"r"(ctx->regs[11]));
	__asm__ volatile("mov x12, %0" ::"r"(ctx->regs[12]));
	__asm__ volatile("mov x13, %0" ::"r"(ctx->regs[13]));
	__asm__ volatile("mov x14, %0" ::"r"(ctx->regs[14]));
	__asm__ volatile("mov x15, %0" ::"r"(ctx->regs[15]));
	__asm__ volatile("mov x16, %0" ::"r"(ctx->regs[16]));
	__asm__ volatile("mov x17, %0" ::"r"(ctx->regs[17]));
	__asm__ volatile("mov x18, %0" ::"r"(ctx->regs[18]));
	__asm__ volatile("mov x19, %0" ::"r"(ctx->regs[19]));
	__asm__ volatile("mov x20, %0" ::"r"(ctx->regs[20]));
	__asm__ volatile("mov x21, %0" ::"r"(ctx->regs[21]));
	__asm__ volatile("mov x22, %0" ::"r"(ctx->regs[22]));
	__asm__ volatile("mov x23, %0" ::"r"(ctx->regs[23]));
	__asm__ volatile("mov x24, %0" ::"r"(ctx->regs[24]));
	__asm__ volatile("mov x25, %0" ::"r"(ctx->regs[25]));
	__asm__ volatile("mov x26, %0" ::"r"(ctx->regs[26]));
	__asm__ volatile("mov x27, %0" ::"r"(ctx->regs[27]));
	__asm__ volatile("mov x28, %0" ::"r"(ctx->regs[28]));
	__asm__ volatile("mov x29, %0" ::"r"(ctx->regs[29]));
	__asm__ volatile("mov x30, %0" ::"r"(ctx->regs[30]));
	__asm__ volatile("ldp x0, x1, %0" ::"rm"(ctx->regs[0]));

	__asm__ volatile("eret");
}