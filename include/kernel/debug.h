#ifndef _KERNEL_DEBUG_H
#define _KERNEL_DEBUG_H

#include <kernel/thread.h>

void enable_hw_debugger();

void disable_hw_debugger();

void user_debug_software_step_handler(thread_t *thread);

#endif