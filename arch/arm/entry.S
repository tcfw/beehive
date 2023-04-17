.comm stack, 0x20000 // Reserve 128k stack in the BSS (64k for irq)

_start:
    .globl _start
    ldr sp, =stack+0x20000 // Set up the stack
    bl kernel_main // Jump to the main function
1:
    b 1b // Halt