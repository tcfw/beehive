#ifndef _KERNEL_INITPROC_H
#define _KERNEL_INITPROC_H

#include <kernel/stdint.h>
#include <kernel/elf.h>

extern char _binary_init_start;
extern char _binary_init_end;

static inline struct elf_hdr_64_t* initproc_elf_hdr() {
	return (struct elf_hdr_64_t*)&_binary_init_start;
}

int load_initproc();

#endif