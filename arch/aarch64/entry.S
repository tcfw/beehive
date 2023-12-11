.section .boot.text, "a"
_start:
    .globl _start

    //TODO(tcfw) check if in EL1

    ADRP x1, init_vm_map
    ADD  x1, x1, #:lo12:init_vm_map
    MSR TTBR0_EL1, x1
    LDR x1, init_tcr
    MSR TCR_EL1, x1
    LDR x1, init_mair
    MSR MAIR_EL1, x1

    MRS x1, SCTLR_EL1
    ORR x1, x1, #1 //Enable MMU
    MSR SCTLR_EL1, x1

    ISB

    MRS x10, MPIDR_EL1
    AND x10, x10, #0xFFFF                 // Mask off to leave Aff0 and Aff1
    CBZ x10, boot_j                       // If not *.*.0.0, then wait
    LDR x1, secondary_boot_addr
    BR x1

boot_j:
    LDR x1, boot_addr
    BR x1

boot_addr: .quad boot
secondary_boot_addr: .quad secondary_boot
init_tcr: .quad 0x1480543514
init_mair: .quad 0x4404FF
    .balign 4096
init_vm_map: 
    // L0 (0@512GB):
    .quad 0x4000000000000701
    // L0 (15@512GB):
    .quad 0x0000000040102701
    .quad 0x0000000040102701
    .quad 0x0000000040102701
    .quad 0x0000000040102701
    .quad 0x0000000040102701
    .quad 0x0000000040102701
    .quad 0x0000000040102701
    .quad 0x0000000040102701
    .quad 0x0000000040102701
    .quad 0x0000000040102701
    .quad 0x0000000040102701
    .quad 0x0000000040102701
    .quad 0x0000000040102701
    .quad 0x0000000040102701
    .quad 0x0000000040102701
    .quad 0x0000000040102701
    //.fill 4080
    .balign 4096

.section .text
boot:
    .globl boot
    // Set Stackpointer
    ADRP x10, stack
    ADD  x10, x10, #:lo12:stack
    MOV sp, x10

    .globl  kernel_main
    B       kernel_main          // Jump to the main function

secondary_boot:
    .globl secondary_boot
    WFE
    
    MRS X0, MPIDR_EL1
    AND x0, x0, #0xFFFF //Get CPU ID

    .global cpu_spin_table
    LDR x2, =cpu_spin_table
    
    LDR X1, [X2, X0, LSL #3] //Get cpu_spin_table[cpu_id]

    CBZ X1, secondary_boot //If not empty, set sp = cpu_spin_table[cpu_id]

    MOV sp, X1
    B kernel_main2

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

push_elx_sp:
    MRS x0, SPSR_EL1
    AND x0, x0, #1
    CMP x0, #0
    BNE _el1sp

    MRS x0, SP_EL0
    b _elxspstore

_el1sp:
    MOV x0, SP
_elxspstore:
    STR x0, [sp, #-8]!

    ret

pull_elx_sp:
    MRS x0, SPSR_EL1
    AND x0, x0, #1
    CMP x0, #0
    BNE _el1sp_p

    LDR x0, [sp], #8
    MSR SP_EL0, x0
    b _elxsp_end

_el1sp_p:
    LDR x0, [sp], #8
    MOV SP, x0
_elxsp_end:

    ret

.macro kexp_trapframe_push
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
    
    MRS x0, ELR_EL1
    STR x0, [sp, #-8]!

    bl push_elx_sp

    MRS x0, SPSR_EL1
    STR x0, [sp, #-8]!
    MOV x0, sp
.endm

.macro kexp_trapframe_pull
    LDR x0, [sp], #8
    MSR SPSR_EL1, x0

    bl pull_elx_sp

    LDR x0, [sp], #8
    MSR ELR_EL1, x0

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
.endm

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

    kexp_trapframe_push

    .global k_exphandler_sync
    bl k_exphandler_sync

    kexp_trapframe_pull

    ERET

syscall_entry:
    LDP x0, x1, [sp], #16
    LDP x2, x3, [sp], #16

    kexp_trapframe_push

    .global k_exphandler_swi_entry
    bl k_exphandler_swi_entry

    kexp_trapframe_pull

    ERET

.global k_exphandler_irq_entry
k_exphandler_irq_entry:
    kexp_trapframe_push

    .globl k_exphandler_irq
    bl k_exphandler_irq
    kexp_trapframe_pull

    ERET

.global k_exphandler_fiq_entry
k_exphandler_fiq_entry:
    kexp_trapframe_push


    .globl k_exphandler_fiq
    bl k_exphandler_fiq
    
    kexp_trapframe_pull

    ERET

syscall1:
    .globl syscall1
syscall2:
    .globl syscall2
syscall3:
    .globl syscall3
syscall4:
    .globl syscall4
    SVC #0
    RET
