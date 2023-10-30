#include <kernel/context.h>
#include <kernel/arch.h>
#include <kernel/strings.h>
#include "regs.h"

void init_context(context_t *ctx)
{
	// set sane values for SPSR
	ctx->spsr = (SPSR_Z | SPSR_PAN | // SPSR_IL |
				 SPSR_M_EL0);
	ctx->sp = 0;
	ctx->pc = 0;

	memset(ctx->regs, 0, sizeof(ctx->regs));
}

void kthread_context(context_t *ctx, void *data)
{
	ctx->spsr = SPSR_M_EL1;

	if (data)
		ctx->regs[0] = (uint64_t)data;
}

void save_to_context(context_t *ctx, uintptr_t trapFrame)
{
	uint64_t *reg = trapFrame;

	ctx->spsr = *reg++;
	ctx->sp = *reg++;
	ctx->pc = *reg++;

	// registers x0-x30
	for (int i = 0; i <= 30; i++)
		ctx->regs[i] = *reg++;

	// // processor states
	// uint64_t elr, sp, spsr = 0;
	// __asm__ volatile("mrs %0, ELR_EL1"
	// 				 : "=r"(elr));
	// __asm__ volatile("mrs %0, SP_EL0"
	// 				 : "=r"(sp));
	// __asm__ volatile("mrs %0, SPSR_EL1"
	// 				 : "=r"(spsr));
	// ctx->pc = elr;
	// ctx->sp = sp;
	// ctx->spsr = spsr;

	// fp registers
	__asm__ volatile("fmov %0, d0"
					 : "=rm"(ctx->fpregs[0]));
	__asm__ volatile("fmov %0, v0.d[1]"
					 : "=rm"(ctx->fpregs[1]));
	__asm__ volatile("fmov %0, d1"
					 : "=rm"(ctx->fpregs[2]));
	__asm__ volatile("fmov %0, v1.d[1]"
					 : "=rm"(ctx->fpregs[3]));
	__asm__ volatile("fmov %0, d2"
					 : "=rm"(ctx->fpregs[4]));
	__asm__ volatile("fmov %0, v2.d[1]"
					 : "=rm"(ctx->fpregs[5]));
	__asm__ volatile("fmov %0, d3"
					 : "=rm"(ctx->fpregs[6]));
	__asm__ volatile("fmov %0, v3.d[1]"
					 : "=rm"(ctx->fpregs[7]));
	__asm__ volatile("fmov %0, d4"
					 : "=rm"(ctx->fpregs[8]));
	__asm__ volatile("fmov %0, v4.d[1]"
					 : "=rm"(ctx->fpregs[9]));
	__asm__ volatile("fmov %0, d5"
					 : "=rm"(ctx->fpregs[10]));
	__asm__ volatile("fmov %0, v5.d[1]"
					 : "=rm"(ctx->fpregs[11]));
	__asm__ volatile("fmov %0, d6"
					 : "=rm"(ctx->fpregs[12]));
	__asm__ volatile("fmov %0, v6.d[1]"
					 : "=rm"(ctx->fpregs[13]));
	__asm__ volatile("fmov %0, d7"
					 : "=rm"(ctx->fpregs[14]));
	__asm__ volatile("fmov %0, v7.d[1]"
					 : "=rm"(ctx->fpregs[15]));
	__asm__ volatile("fmov %0, d8"
					 : "=rm"(ctx->fpregs[16]));
	__asm__ volatile("fmov %0, v8.d[1]"
					 : "=rm"(ctx->fpregs[17]));
	__asm__ volatile("fmov %0, d9"
					 : "=rm"(ctx->fpregs[18]));
	__asm__ volatile("fmov %0, v9.d[1]"
					 : "=rm"(ctx->fpregs[19]));
	__asm__ volatile("fmov %0, d10"
					 : "=rm"(ctx->fpregs[20]));
	__asm__ volatile("fmov %0, v10.d[1]"
					 : "=rm"(ctx->fpregs[21]));
	__asm__ volatile("fmov %0, d11"
					 : "=rm"(ctx->fpregs[22]));
	__asm__ volatile("fmov %0, v11.d[1]"
					 : "=rm"(ctx->fpregs[23]));
	__asm__ volatile("fmov %0, d12"
					 : "=rm"(ctx->fpregs[24]));
	__asm__ volatile("fmov %0, v12.d[1]"
					 : "=rm"(ctx->fpregs[25]));
	__asm__ volatile("fmov %0, d13"
					 : "=rm"(ctx->fpregs[26]));
	__asm__ volatile("fmov %0, v13.d[1]"
					 : "=rm"(ctx->fpregs[27]));
	__asm__ volatile("fmov %0, d14"
					 : "=rm"(ctx->fpregs[28]));
	__asm__ volatile("fmov %0, v14.d[1]"
					 : "=rm"(ctx->fpregs[29]));
	__asm__ volatile("fmov %0, d15"
					 : "=rm"(ctx->fpregs[30]));
	__asm__ volatile("fmov %0, v15.d[1]"
					 : "=rm"(ctx->fpregs[31]));
	__asm__ volatile("fmov %0, d16"
					 : "=rm"(ctx->fpregs[32]));
	__asm__ volatile("fmov %0, v16.d[1]"
					 : "=rm"(ctx->fpregs[33]));
	__asm__ volatile("fmov %0, d17"
					 : "=rm"(ctx->fpregs[34]));
	__asm__ volatile("fmov %0, v17.d[1]"
					 : "=rm"(ctx->fpregs[35]));
	__asm__ volatile("fmov %0, d18"
					 : "=rm"(ctx->fpregs[36]));
	__asm__ volatile("fmov %0, v18.d[1]"
					 : "=rm"(ctx->fpregs[37]));
	__asm__ volatile("fmov %0, d19"
					 : "=rm"(ctx->fpregs[38]));
	__asm__ volatile("fmov %0, v19.d[1]"
					 : "=rm"(ctx->fpregs[39]));
	__asm__ volatile("fmov %0, d20"
					 : "=rm"(ctx->fpregs[40]));
	__asm__ volatile("fmov %0, v20.d[1]"
					 : "=rm"(ctx->fpregs[41]));
	__asm__ volatile("fmov %0, d21"
					 : "=rm"(ctx->fpregs[42]));
	__asm__ volatile("fmov %0, v21.d[1]"
					 : "=rm"(ctx->fpregs[43]));
	__asm__ volatile("fmov %0, d22"
					 : "=rm"(ctx->fpregs[44]));
	__asm__ volatile("fmov %0, v22.d[1]"
					 : "=rm"(ctx->fpregs[45]));
	__asm__ volatile("fmov %0, d23"
					 : "=rm"(ctx->fpregs[46]));
	__asm__ volatile("fmov %0, v23.d[1]"
					 : "=rm"(ctx->fpregs[47]));
	__asm__ volatile("fmov %0, d24"
					 : "=rm"(ctx->fpregs[48]));
	__asm__ volatile("fmov %0, v24.d[1]"
					 : "=rm"(ctx->fpregs[49]));
	__asm__ volatile("fmov %0, d25"
					 : "=rm"(ctx->fpregs[50]));
	__asm__ volatile("fmov %0, v25.d[1]"
					 : "=rm"(ctx->fpregs[51]));
	__asm__ volatile("fmov %0, d26"
					 : "=rm"(ctx->fpregs[52]));
	__asm__ volatile("fmov %0, v26.d[1]"
					 : "=rm"(ctx->fpregs[53]));
	__asm__ volatile("fmov %0, d27"
					 : "=rm"(ctx->fpregs[54]));
	__asm__ volatile("fmov %0, v27.d[1]"
					 : "=rm"(ctx->fpregs[55]));
	__asm__ volatile("fmov %0, d28"
					 : "=rm"(ctx->fpregs[56]));
	__asm__ volatile("fmov %0, v28.d[1]"
					 : "=rm"(ctx->fpregs[57]));
	__asm__ volatile("fmov %0, d29"
					 : "=rm"(ctx->fpregs[58]));
	__asm__ volatile("fmov %0, v29.d[1]"
					 : "=rm"(ctx->fpregs[59]));
	__asm__ volatile("fmov %0, d30"
					 : "=rm"(ctx->fpregs[60]));
	__asm__ volatile("fmov %0, v30.d[1]"
					 : "=rm"(ctx->fpregs[61]));
	__asm__ volatile("fmov %0, d31"
					 : "=rm"(ctx->fpregs[62]));
	__asm__ volatile("fmov %0, v31.d[1]"
					 : "=rm"(ctx->fpregs[63]));
}

void set_to_context(context_t *ctx, uintptr_t trapFrame)
{
	__asm__ volatile("fmov d0, %0" ::"r"(ctx->fpregs[0]));
	__asm__ volatile("fmov v0.d[1], %0" ::"r"(ctx->fpregs[1]));
	__asm__ volatile("fmov d1, %0" ::"r"(ctx->fpregs[2]));
	__asm__ volatile("fmov v1.d[1], %0" ::"r"(ctx->fpregs[3]));
	__asm__ volatile("fmov d2, %0" ::"r"(ctx->fpregs[4]));
	__asm__ volatile("fmov v2.d[1], %0" ::"r"(ctx->fpregs[5]));
	__asm__ volatile("fmov d3, %0" ::"r"(ctx->fpregs[6]));
	__asm__ volatile("fmov v3.d[1], %0" ::"r"(ctx->fpregs[7]));
	__asm__ volatile("fmov d4, %0" ::"r"(ctx->fpregs[8]));
	__asm__ volatile("fmov v4.d[1], %0" ::"r"(ctx->fpregs[9]));
	__asm__ volatile("fmov d5, %0" ::"r"(ctx->fpregs[10]));
	__asm__ volatile("fmov v5.d[1], %0" ::"r"(ctx->fpregs[11]));
	__asm__ volatile("fmov d6, %0" ::"r"(ctx->fpregs[12]));
	__asm__ volatile("fmov v6.d[1], %0" ::"r"(ctx->fpregs[13]));
	__asm__ volatile("fmov d7, %0" ::"r"(ctx->fpregs[14]));
	__asm__ volatile("fmov v7.d[1], %0" ::"r"(ctx->fpregs[15]));
	__asm__ volatile("fmov d8, %0" ::"r"(ctx->fpregs[16]));
	__asm__ volatile("fmov v8.d[1], %0" ::"r"(ctx->fpregs[17]));
	__asm__ volatile("fmov d9, %0" ::"r"(ctx->fpregs[18]));
	__asm__ volatile("fmov v9.d[1], %0" ::"r"(ctx->fpregs[19]));
	__asm__ volatile("fmov d10, %0" ::"r"(ctx->fpregs[20]));
	__asm__ volatile("fmov v10.d[1], %0" ::"r"(ctx->fpregs[21]));
	__asm__ volatile("fmov d11, %0" ::"r"(ctx->fpregs[22]));
	__asm__ volatile("fmov v11.d[1], %0" ::"r"(ctx->fpregs[23]));
	__asm__ volatile("fmov d12, %0" ::"r"(ctx->fpregs[24]));
	__asm__ volatile("fmov v12.d[1], %0" ::"r"(ctx->fpregs[25]));
	__asm__ volatile("fmov d13, %0" ::"r"(ctx->fpregs[26]));
	__asm__ volatile("fmov v13.d[1], %0" ::"r"(ctx->fpregs[27]));
	__asm__ volatile("fmov d14, %0" ::"r"(ctx->fpregs[28]));
	__asm__ volatile("fmov v14.d[1], %0" ::"r"(ctx->fpregs[29]));
	__asm__ volatile("fmov d15, %0" ::"r"(ctx->fpregs[30]));
	__asm__ volatile("fmov v15.d[1], %0" ::"r"(ctx->fpregs[31]));
	__asm__ volatile("fmov d16, %0" ::"r"(ctx->fpregs[32]));
	__asm__ volatile("fmov v16.d[1], %0" ::"r"(ctx->fpregs[33]));
	__asm__ volatile("fmov d17, %0" ::"r"(ctx->fpregs[34]));
	__asm__ volatile("fmov v17.d[1], %0" ::"r"(ctx->fpregs[35]));
	__asm__ volatile("fmov d18, %0" ::"r"(ctx->fpregs[36]));
	__asm__ volatile("fmov v18.d[1], %0" ::"r"(ctx->fpregs[37]));
	__asm__ volatile("fmov d19, %0" ::"r"(ctx->fpregs[38]));
	__asm__ volatile("fmov v19.d[1], %0" ::"r"(ctx->fpregs[39]));
	__asm__ volatile("fmov d20, %0" ::"r"(ctx->fpregs[40]));
	__asm__ volatile("fmov v20.d[1], %0" ::"r"(ctx->fpregs[41]));
	__asm__ volatile("fmov d21, %0" ::"r"(ctx->fpregs[42]));
	__asm__ volatile("fmov v21.d[1], %0" ::"r"(ctx->fpregs[43]));
	__asm__ volatile("fmov d22, %0" ::"r"(ctx->fpregs[44]));
	__asm__ volatile("fmov v22.d[1], %0" ::"r"(ctx->fpregs[45]));
	__asm__ volatile("fmov d23, %0" ::"r"(ctx->fpregs[46]));
	__asm__ volatile("fmov v23.d[1], %0" ::"r"(ctx->fpregs[47]));
	__asm__ volatile("fmov d24, %0" ::"r"(ctx->fpregs[48]));
	__asm__ volatile("fmov v24.d[1], %0" ::"r"(ctx->fpregs[49]));
	__asm__ volatile("fmov d25, %0" ::"r"(ctx->fpregs[50]));
	__asm__ volatile("fmov v25.d[1], %0" ::"r"(ctx->fpregs[51]));
	__asm__ volatile("fmov d26, %0" ::"r"(ctx->fpregs[52]));
	__asm__ volatile("fmov v26.d[1], %0" ::"r"(ctx->fpregs[53]));
	__asm__ volatile("fmov d27, %0" ::"r"(ctx->fpregs[54]));
	__asm__ volatile("fmov v27.d[1], %0" ::"r"(ctx->fpregs[55]));
	__asm__ volatile("fmov d28, %0" ::"r"(ctx->fpregs[56]));
	__asm__ volatile("fmov v28.d[1], %0" ::"r"(ctx->fpregs[57]));
	__asm__ volatile("fmov d29, %0" ::"r"(ctx->fpregs[58]));
	__asm__ volatile("fmov v29.d[1], %0" ::"r"(ctx->fpregs[59]));
	__asm__ volatile("fmov d30, %0" ::"r"(ctx->fpregs[60]));
	__asm__ volatile("fmov v30.d[1], %0" ::"r"(ctx->fpregs[61]));
	__asm__ volatile("fmov d31, %0" ::"r"(ctx->fpregs[62]));
	__asm__ volatile("fmov v31.d[1], %0" ::"r"(ctx->fpregs[63]));

	uint64_t *reg = (uint64_t *)trapFrame;

	// setup pc, sp & pstate
	// __asm__ volatile("msr ELR_EL1, %0" ::"r"(ctx->pc));
	// __asm__ volatile("msr SP_EL0, %0" ::"r"(ctx->sp));
	// __asm__ volatile("msr SPSR_EL1, %0" ::"r"(ctx->spsr));

	*reg++ = ctx->spsr;
	*reg++ = ctx->sp;
	*reg++ = ctx->pc;

	// registers x0-x30
	for (int i = 0; i <= 30; i++)
		*reg++ = ctx->regs[i];
}

void switch_to_context(context_t *ctx)
{
	// setup pc, sp & pstate
	__asm__ volatile("msr ELR_EL1, %0" ::"r"(ctx->pc));
	__asm__ volatile("msr SP_EL0, %0" ::"r"(ctx->sp));
	__asm__ volatile("msr SPSR_EL1, %0" ::"r"(ctx->spsr));

	__asm__ volatile("ldp x2, x3, %0" ::"rm"(ctx->regs[2]));
	__asm__ volatile("ldp x4, x5, %0" ::"rm"(ctx->regs[4]));
	__asm__ volatile("ldp x6, x7, %0" ::"rm"(ctx->regs[6]));
	__asm__ volatile("ldp x8, x9, %0" ::"rm"(ctx->regs[8]));
	__asm__ volatile("ldp x10, x11, %0" ::"rm"(ctx->regs[10]));
	__asm__ volatile("ldp x12, x13, %0" ::"rm"(ctx->regs[12]));
	__asm__ volatile("ldp x14, x15, %0" ::"rm"(ctx->regs[14]));
	__asm__ volatile("ldp x16, x17, %0" ::"rm"(ctx->regs[16]));
	__asm__ volatile("ldp x18, x19, %0" ::"rm"(ctx->regs[18]));
	__asm__ volatile("ldp x20, x21, %0" ::"rm"(ctx->regs[20]));
	__asm__ volatile("ldp x22, x23, %0" ::"rm"(ctx->regs[22]));
	__asm__ volatile("ldp x24, x25, %0" ::"rm"(ctx->regs[24]));
	__asm__ volatile("ldp x26, x27, %0" ::"rm"(ctx->regs[26]));
	__asm__ volatile("ldp x28, x29, %0" ::"rm"(ctx->regs[28]));
	__asm__ volatile("ldr x30, %0" ::"rm"(ctx->regs[30]));
	__asm__ volatile("ldp x0, x1, %0" ::"rm"(ctx->regs[0]));

	__asm__ volatile("eret");
}