#ifndef _KERNEL_ARCH_BARRIERS_H
#define _KERNEL_ARCH_BARRIERS_H

#define arch_mb asm volatile("dsb ish" ::: "memory");
#define arch_ib asm volatile("isb" ::: "memory");

#endif