#ifndef _KERNEL_UMM_H
#define _KERNEL_UMM_H

#include <kernel/stdint.h>
#include <kernel/vm.h>

enum UserDataAbortOp
{
	USER_DATA_ABORT_READ = 0,
	USER_DATA_ABORT_WRITE = 1,
};

vm_mapping *has_mapping(thread_t *thread, uintptr_t addr, size_t length);

void user_data_abort(uintptr_t daddr, enum UserDataAbortOp op, uintptr_t pc);

int vm_alloc_lazy_mapping(thread_t *thread, vm_mapping *map, uint64_t alloc_size);

int64_t actualise_lazy_reservation(thread_t *thread, vm_mapping *map, int flags);

uint64_t lazy_mem_map(thread_t *thread, uintptr_t addr, size_t length, int flags);

#endif