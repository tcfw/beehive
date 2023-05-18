_start:
    .globl _start
    MRS      x0, MPIDR_EL1
    AND      x0, x0, #0xFFFF                   // Mask off to leave Aff0 and Aff1
    CBZ      x0, boot                          // If not *.*.0.0, then go to sleep

    B        .

boot:
    // Set Stackpointer
    LDR x0, =stack+0x20000
    MOV sp, x0

    ISB

    .globl  kernel_main
    BL      kernel_main          // Jump to the main function

halt_loop:
    WFI
    B halt_loop

.set sync_stack, intstack

.global k_exphandler_sync_entry
k_exphandler_sync_entry:
    //switch stacks and save for return 
    MOV x29, SP
    LDR x28, =sync_stack 
    MOV SP, x28
    STR	x29, [sp, #-16]!
    STR	lr, [sp, #-16]!

    .global k_exphandler_sync
    bl k_exphandler_sync

    //restore 
    LDR lr, [sp], #16
    LDR x29, [sp], #16
    MOV sp, x29

    ERET

.global k_exphandler_irq_entry
k_exphandler_irq_entry:
    MOV x29, SP
    LDR x28, =intstack
    MOV SP, x28
    STR	x29, [sp, #-16]!
    STR	lr, [sp, #-16]!

    .globl k_exphandler_irq
    bl k_exphandler_irq
    
    LDR lr, [sp], #16
    LDR x29, [sp], #16
    MOV sp, x29

    ERET

.global k_exphandler_fiq_entry
k_exphandler_fiq_entry:
    MOV x29, SP
    LDR x28, =intstack
    MOV SP, x28
    STR	x29, [sp, #-16]!
    STR	lr, [sp, #-16]!

    .globl k_exphandler_fiq
    bl k_exphandler_fiq
    
    LDR lr, [sp], #16
    LDR x29, [sp], #16
    MOV sp, x29

    ERET