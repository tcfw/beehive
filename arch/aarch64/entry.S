_start:
    .globl _start
    MRS      x10, MPIDR_EL1
    AND      x10, x10, #0xFFFF                   // Mask off to leave Aff0 and Aff1
    CBZ      x10, boot                          // If not *.*.0.0, then wait
    B        secondary_boot

boot:
    // Set Stackpointer
    LDR x10, =stack
    MOV sp, x10

    ISB

    .globl  kernel_main
    BL      kernel_main          // Jump to the main function

halt_loop:
    .globl halt_loop
    WFE
    B halt_loop

address_xlate_read:
    .globl address_xlate_read
    AT S1E1R, X0
    MRS X0, PAR_EL1
    RET

address_xlate_write:
    .globl address_xlate_write
    AT S1E1W, X0
    MRS X0, PAR_EL1
    RET

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

.global k_exphandler_sync_entry
k_exphandler_sync_entry:
    STP x2, x3, [sp, #-16]!
    STP x0, x1, [sp, #-16]!

    // syscall
    MRS x0, ESR_EL1
    LSR x0, x0, #26
    CMP x0, #0x15
    b.eq syscall_entry

    //other sync exception
    LDP x0, x1, [sp], #16
    LDP x2, x3, [sp], #16
    
    STR x30, [sp, #-8]!
    STP x28, x29, [sp, #-16]!
    STP x26, x27, [sp, #-16]!
    STP x24, x25, [sp, #-16]!
    STP x22, x23, [sp, #-16]!
    STP x20, x21, [sp, #-16]!
    STP x18, x19, [sp, #-16]!
    STP x16, x17, [sp, #-16]!
    STP x14, x15, [sp, #-16]!
    STP x12, x13, [sp, #-16]!
    STP x10, x11, [sp, #-16]!
    STP x8, x9, [sp, #-16]!
    STP x6, x7, [sp, #-16]!
    STP x4, x5, [sp, #-16]!
    STP x2, x3, [sp, #-16]!
    STP x0, x1, [sp, #-16]!

    mov x0, sp

    .global k_exphandler_sync
    bl k_exphandler_sync

    LDP x0, x1, [sp], #16
    LDP x2, x3, [sp], #16
    LDP x4, x5, [sp], #16
    LDP x6, x7, [sp], #16
    LDP x8, x9, [sp], #16
    LDP x10, x11, [sp], #16
    LDP x12, x13, [sp], #16
    LDP x14, x15, [sp], #16
    LDP x16, x17, [sp], #16
    LDP x18, x19, [sp], #16
    LDP x20, x21, [sp], #16
    LDP x22, x23, [sp], #16
    LDP x24, x25, [sp], #16
    LDP x26, x27, [sp], #16
    LDP x28, x29, [sp], #16
    LDR x30, [sp], #8

    ERET

syscall_entry:
    LDP x0, x1, [sp], #16
    LDP x2, x3, [sp], #16

	STR x30, [sp, #-8]!
    STP x28, x29, [sp, #-16]!
    STP x26, x27, [sp, #-16]!
    STP x24, x25, [sp, #-16]!
    STP x22, x23, [sp, #-16]!
    STP x20, x21, [sp, #-16]!
    STP x18, x19, [sp, #-16]!
    STP x16, x17, [sp, #-16]!
    STP x14, x15, [sp, #-16]!
    STP x12, x13, [sp, #-16]!
    STP x10, x11, [sp, #-16]!
    STP x8, x9, [sp, #-16]!
    STP x6, x7, [sp, #-16]!
    STP x4, x5, [sp, #-16]!
    STP x2, x3, [sp, #-16]!
	STR x1, [sp, #-8]!

    MRS x26, ELR_EL1
	STR x26, [sp, #-8]!
    MRS x26, SPSR_EL1
	STR x26, [sp, #-8]!

    .global k_exphandler_swi_entry
    bl k_exphandler_swi_entry

    LDR x26, [sp], #8
    MSR SPSR_EL1, x26
    LDR x26, [sp], #8
    MSR ELR_EL1, x26

    LDR x1, [sp], #8
    LDP x2, x3, [sp], #16
    LDP x4, x5, [sp], #16
    LDP x6, x7, [sp], #16
    LDP x8, x9, [sp], #16
    LDP x10, x11, [sp], #16
    LDP x12, x13, [sp], #16
    LDP x14, x15, [sp], #16
    LDP x16, x17, [sp], #16
    LDP x18, x19, [sp], #16
    LDP x20, x21, [sp], #16
    LDP x22, x23, [sp], #16
    LDP x24, x25, [sp], #16
    LDP x26, x27, [sp], #16
    LDP x28, x29, [sp], #16
    LDR x30, [sp], #8

    ERET

.global k_exphandler_irq_entry
k_exphandler_irq_entry:
    STR	lr, [sp, #-8]!

    .globl k_exphandler_irq
    bl k_exphandler_irq
    
    LDR lr, [sp], #8

    ERET

.global k_exphandler_fiq_entry
k_exphandler_fiq_entry:
    STR	lr, [sp, #-8]!

    .globl k_exphandler_fiq
    bl k_exphandler_fiq
    
    LDR lr, [sp], #8

    ERET