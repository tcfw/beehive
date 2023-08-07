// Copy from user space to kernel 
// note access_ok should have been called 
// already to ensure valid read range
// x0 - src
// x1 - dst
// x2 - length
_copy_from_user:
	.globl _copy_from_user

loop1:
	cmp x2, #0
	b.eq ret2

	//TODO(tcfw) use alignment and length checks to do 8-byte xfers

	ldtrb w9, [x0] // unprivileged
	strb w9, [x1]  // privileged
	add x0, x0, #1
	add x1, x1, #1
	sub x2, x2, #1

	b loop1

ret2:
	ret

// Copy to user space from kernel 
// note access_ok should have been called 
// already to ensure valid write range
// x0 - src
// x1 - dst
// x2 - length
_copy_to_user:
	.globl _copy_to_user

loop3:
	cmp x2, #0
	b.eq ret4

	//TODO(tcfw) use alignment and length checks to do 8-byte xfers

	ldrb w9, [x0]   // privileged
	sttrb w9, [x1]  // unprivileged
	add x0, x0, #1
	add x1, x1, #1
	sub x2, x2, #1

	b loop3

ret4:
	ret