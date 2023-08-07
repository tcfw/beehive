#ifndef _ARCH_REGS_H
#define _ARCH_REGS_H

#define ESR_EXCEPTION_CLASS(xrq) ((xrq & 0xFC000000) >> 26)

#define SPSR_Z (1ULL << 30)
#define SPSR_SSB (1ULL << 23)
#define SPSR_PAN (1ULL << 22)
#define SPSR_SS (1ULL << 21)
#define SPSR_IL (1ULL << 20)
#define SPSR_E (1ULL << 9)
#define SPSR_A (1ULL << 8)
#define SPSR_I (1ULL << 7)
#define SPSR_F (1ULL << 6)
#define SPSR_M_AARCH32 (1ULL << 4)
#define SPSR_M_EL0 (0b0000)
#define SPSR_M_EL1 (0b0100)
#define SPSR_M_SP (0b0001)

#endif