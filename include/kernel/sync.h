#ifndef _KERNEL_SYNC_H
#define _KERNEL_SYNC_H

#include <stdint.h>
#include <stdbool.h>

typedef volatile uint32_t spinlock_t;

// Initialize the lock to be available
void spinlock_init(spinlock_t *lock);

// Acquire the lock
void spinlock_acquire(spinlock_t *lock);

// Release the lock
void spinlock_release(spinlock_t *lock);

// Check if the lock is currently held
bool spinlock_is_locked(spinlock_t *lock);

#endif