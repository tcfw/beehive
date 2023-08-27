/*
while(1) {
	writeconsole((void *)0x1002, 1);
}
*/

user_init:
	.globl user_init
	MOV x0, #0x1002
	MOV x1, #1
	SVC #131
	B user_init