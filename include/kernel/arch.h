#ifndef _KERNEL_ARCH_H
#define _KERNEL_ARCH_H

#include "stdint.h"
#include <kernel/context.h>

// Initiate arch specific requirements
void arch_init(void);

// Initiate a single core
void core_init(void);

// Turn off arch specific components
void arch_poweroff();

// Get the current processor/cpu ID
uint32_t cpu_id();

// Get the current processor/cpu brand identifier
uint64_t cpu_brand();

// Wait for interrupt
void wfi();

// Wake up all other cores
void wake_cores(void);

// Stop all other cores
void stop_cores(void);

// Switch to a new context. Assumes no other context has been set up in a
// trap frame
void switch_to_context(context_t *ctx);

// Save a context give the trap frame location. Assume the pointer of the
// trap frame contains all required registers in order
void save_to_context(context_t *ctx, uintptr_t trapFrame);

// Sets to a new context. Assumes there is already a context in the trap
// frame at the given location and overrides the existing context
void set_to_context(context_t *ctx, uintptr_t trapFrame);

// Get the max address available in RAM
uintptr_t ram_max(void);

// Get size of available in RAM
uintptr_t ram_size(void);

// Switch to the wait task
void wait_task(void);

// init register clocks sources
void registerClocks(void);

#endif