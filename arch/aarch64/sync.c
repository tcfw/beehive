#include <kernel/sync.h>
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