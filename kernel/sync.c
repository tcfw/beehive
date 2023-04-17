#include <kernel/sync.h>

// Initialize the lock to be available
void spinlock_init(spinlock_t *lock)
{
	*lock = 0;
}

// Acquire the lock
void spinlock_acquire(spinlock_t *lock)
{
	while (__atomic_test_and_set(lock, __ATOMIC_ACQUIRE))
		; // spin
}

// Release the lock
void spinlock_release(spinlock_t *lock)
{
	__atomic_clear(lock, __ATOMIC_RELEASE);
}

// Check if the lock is currently held
bool spinlock_is_locked(spinlock_t *lock)
{
	return (*lock != 0);
}