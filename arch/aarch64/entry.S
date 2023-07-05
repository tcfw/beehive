_start:
    .globl _start
    MRS      x0, MPIDR_EL1
    AND      x0, x0, #0xFFFF                   // Mask off to leave Aff0 and Aff1
    CBZ      x0, boot                          // If not *.*.0.0, then wait
    B        secondary_boot

boot:
    // Set Stackpointer
    LDR x0, =stack+0x20000
    MOV sp, x0

    ISB

    .globl  kernel_main
    BL      kernel_main          // Jump to the main function

halt_loop:
    .globl halt_loop
    WFE
    B halt_loop

.global secondary_boot
secondary_boot:
    WFE
    
    MRS X0, MPIDR_EL1
    AND x0, x0, #0xFFFF //Get CPU ID

    .global cpu_spin_table
    LDR x2, =cpu_spin_table
    
    LDR X1, [X2, X0, LSL #3] //Get cpu_spin_table[cpu_id]

    CBZ X1, secondary_boot //If not empty, set sp = cpu_spin_table[cpu_id]

    MOV sp, X1
    B kernel_main2

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