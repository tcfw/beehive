/*
while(1) {
	writeconsole((void *)0x1002, 1);
}
*/

user_init:
	.globl user_init
	MOV x0, #131 //writeconsole
	MOV x1, #0x1014 //user_msg
	MOV x2, #7
	SVC #0
	B user_init

user_msg:
	.string "Hello!\0"