/*
 * Acquire lock using Compare and Swap instruction.
 *
 * Compare for 0 with acquire semantics, and swap 1. If failed to acquire, use
 * load exclusive semantics to monitor the address and enter WFE.
 *
 * void spin_lock(spinlock_t *lock);
 */
spinlock_acquire:
	.globl	spinlock_acquire

	/*
	mov	w2, #1
1:	mov	w1, wzr
2:	casa	w1, w2, [x0]
	cbz	w1, 3f
	ldxr	w1, [x0]
	cbz	w1, 2b
	wfe
	b	1b
3:
	ret
	*/

	mov	w2, #1
	sevl
l1:	wfe
l2:	ldaxr	w1, [x0]
	cbnz	w1, l1
	stxr	w1, w2, [x0]
	cbnz	w1, l2
	ret

/*
 * Release lock previously acquired by spin_lock.
 *
 * Use store-release to unconditionally clear the spinlock variable.
 * Store operation generates an event to all cores waiting in WFE
 * when address is monitored by the global monitor.
 *
 * void spin_unlock(spinlock_t *lock);
 */
spinlock_release:
	.globl	spinlock_release
	stlr	wzr, [x0]
	ret