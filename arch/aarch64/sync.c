#include <kernel/sync.h>
#include <kernel/irq.h>
#include <stdbool.h>

// Initialize the lock to be available
void spinlock_init(spinlock_t *lock)
{
	*lock = 0;
}

// Check if the lock is currently held
bool spinlock_is_locked(spinlock_t *lock)
{
	return lock != 0;
}

// Acquire the lock and disable IRQ
void spinlock_acquire_irq(spinlock_t *lock)
{
	disable_irq();
	spinlock_acquire(lock);
}

// Release the lock from disabled IRQ
void spinlock_release_irq(spinlock_t *lock)
{
	spinlock_release(lock);
	enable_irq();
}