user_init:
	.globl user_init
	MOV x0, #1
	MOV x1, #2
	SVC #0