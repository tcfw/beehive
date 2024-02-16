#include <kernel/sync.h>
#include <kernel/irq.h>
#include <kernel/stdbool.h>

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
int spinlock_acquire_irq(spinlock_t *lock)
{
	int daif = 0;
	__asm__ volatile("MRS %0, DAIF"
					 : "=r"(daif));

	disable_irq();
	spinlock_acquire(lock);

	return daif;
}

// Release the lock from disabled IRQ
void spinlock_release_irq(int state, spinlock_t *lock)
{
	spinlock_release(lock);
	__asm__ volatile("MSR DAIF, %0" ::"r"(state));
}