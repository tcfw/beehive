#include <kernel/sync.h>
#include <stdbool.h>

// Initialize the lock to be available
void spinlock_init(spinlock_t *lock)
{
	*lock = 0;
}

// Acquire the lock
void spinlock_acquire(spinlock_t *lock)
{
	__asm__ volatile(
		"mov	w2, #1 \n\t"
		"sevl \n\t"
		"l1:	wfe \n\t"
		"l2:	ldaxr	w1, [%0] \n\t"
		"cbnz	w1, l1 \n\t"
		"stxr	w1, w2, [%0] \n\t"
		"cbnz	w1, l2 \n\t"
		: "=r"(lock));
}

// Release the lock
void spinlock_release(spinlock_t *lock)
{
	__asm__ volatile(
		"stlr wzr, [%0]"
		: "=r"(lock));
}

// Check if the lock is currently held
bool spinlock_is_locked(spinlock_t *lock)
{
	return lock != 0;
}