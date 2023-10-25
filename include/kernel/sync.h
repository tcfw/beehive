#ifndef _KERNEL_SYNC_H
#define _KERNEL_SYNC_H

#include <stdbool.h>

typedef unsigned int spinlock_t;

// Initialize the lock to be available
void spinlock_init(spinlock_t *lock);

// Acquire the lock
void spinlock_acquire(spinlock_t *lock);

// Release the lock
void spinlock_release(spinlock_t *lock);

// Check if the lock is currently held
bool spinlock_is_locked(spinlock_t *lock);

// Acquire the lock and disable IRQ
void spinlock_acquire_irq(spinlock_t *lock);

// Release the lock from disabled IRQ
void spinlock_release_irq(spinlock_t *lock);

#endif