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

	ldtrb w3, [x0] // unprivileged

	b ok1
	b ret3 //mark_failed_copy will bump to here
	
	ok1:
	strb w3, [x1]  // privileged
	add x0, x0, #1
	add x1, x1, #1
	sub x2, x2, #1

	b loop1

	ret2:
	ret

	ret3:
	mov x0, x2
	isb
	ret

// Copy to user space from kernel 
// note access_ok should have been called 
// already to ensure valid write range
// x0 - src
// x1 - dst
// x2 - length
_copy_to_user:
	.globl _copy_to_user

	loop4:
	cmp x2, #0
	b.eq ret5

	//TODO(tcfw) use alignment and length checks to do 8-byte xfers

	ldrb w3, [x0]   // privileged

	b ok2
	b ret6 //mark_failed_copy will bump to here

	ok2:
	sttrb w3, [x1]  // unprivileged
	add x0, x0, #1
	add x1, x1, #1
	sub x2, x2, #1

	b loop4

	ret5:
	ret

	ret6:
	mov x0, x2
	isb
	ret


mark_failed_copy:
	.globl mark_failed_copy

	//bump exception return an additional 2 instructions
	mrs x12, ELR_EL1
	add x12, x12, #8
	msr ELR_EL1, x12

	ret