.section .boot.text, "a"
_start:
    .globl _start

    //TODO(tcfw) check if in EL1

    MOV x0, #0x3C0
    MSR DAIF, X0
    
    ISB

    ADRP x1, #init_tcr
    ADD  x1, x1, #:lo12:init_tcr
    LDR x1, [x1]
    MSR TCR_EL1, x1

    ADRP x1, #init_mair
    ADD  x1, x1, #:lo12:init_mair
    LDR x1, [x1]
    MSR MAIR_EL1, x1

    ADRP x2, #init_vm_map
    ADD  x2, x2, #:lo12:init_vm_map

    ADRP x3, #l0_vm_map_attrs
    ADD  x3, x3, #:lo12:l0_vm_map_attrs
    LDR x3, [x3]

    //populate init_vm_map L0
    LDR x1, [x2]
    ORR x1, x1, x3
    LDR x3, =0x1
    ORR x1, x1, x3, lsl #63 //NSTable

    //Copy for all records in L0
    MOV x4, #0
    MOV x5, x2
copy_l0:
    STR x1, [x2]
    ADD x4, x4, #1
    ADD x2, x2, #8
    CMP x4, #512
    BNE copy_l0

    MSR TTBR0_EL1, x5

    MRS x1, SCTLR_EL1
    ORR x1, x1, #1 //MMU
    MSR SCTLR_EL1, x1

    IC IALLU
    ISB
    TLBI VMALLE1
    DSB ISH
    ISB

    MRS x10, MPIDR_EL1
    AND x10, x10, #0xFFFF                 // Mask off to leave Aff0 and Aff1
    CBZ x10, boot_j                       // If not *.*.0.0, then wait
    LDR x1, secondary_boot_addr
    BR x1

boot_j:
    LDR x1, boot_addr
    LDR x2, term_entry
    AT S1E1R, x1
    AT S1E1R, x2
    BR x1

boot_addr: .quad boot
secondary_boot_addr: .quad secondary_boot
init_tcr: .quad 0x1480543514
init_mair: .quad 0x4404FF
l0_vm_map_attrs: .quad 0x2F23
term_entry: .quad 0x0000000009000000

    .balign 4096
init_vm_map: 
    // L0 (0@512GB):
    .quad l1_vm_map

    .balign 4096

l1_vm_map:
    .quad 0x0010000000000625 //Device Region 0x0 -> 0x40000000
    .quad 0x0000000040000421 //RAM @ 1Gb - 0x40000000 -> 0x60000000
    .quad 0x0000000060000421 //RAM @ 1Gb - 0x60000000 -> 0x80000000

    .balign 4096

.section .text
boot:
    .globl boot
    // Set Stackpointer
    ADRP x10, #stack
    ADD  x10, x10, #:lo12:stack
    MOV sp, x10

    //Add null stack frame
    SUB sp, sp, #16
    MOV fp, xzr
    MOV lr, xzr //no return

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

syscall0:
    .globl syscall0
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

 switch_to_context:
    .globl switch_to_context

    //ELR
    ldr x1, [x0, #0x2f8]
    msr ELR_EL1, x1

    //SP
    ldr x1, [x0, #0x300]
    msr SP_EL0, x1

    //SPSR
    ldr x1, [x0, #0x308]
    msr SPSR_EL1, x1

    //Registers x2-x30, x0 & x1
    LDP x2, x3, [x0, #0x10]
    LDP x4, x5, [x0, #0x20]
    LDP x6, x7, [x0, #0x30]
    LDP x8, x9, [x0, #0x40]
    LDP x10, x11, [x0, #0x50]
    LDP x12, x13, [x0, #0x60]
    LDP x14, x15, [x0, #0x70]
    LDP x16, x17, [x0, #0x80]
    LDP x18, x19, [x0, #0x90]
    LDP x20, x21, [x0, #0xA0]
    LDP x22, x23, [x0, #0xB0]
    LDP x24, x25, [x0, #0xC0]
    LDP x26, x27, [x0, #0xD0]
    LDP x28, x29, [x0, #0xE0]
    LDR x30, [x0, #0xF0]
    LDP x0, x1, [x0]

    eret

.section .vdso.text, "a"
sigreturn:
    .globl sigreturn
    MOV x0, #108 //sig_return
    SVC #0
