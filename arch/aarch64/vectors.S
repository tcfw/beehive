 .section  VECTORS,"ax"
  .align 12
 
  .global vectors
vectors:

  .global k_exphandler_sync_entry
  .global k_exphandler_irq_entry
  .global k_exphandler_fiq_entry
  .global k_exphandler_serror_entry
  .balign 2048

// ------------------------------------------------------------
// Current EL with SP0
// ------------------------------------------------------------

  .balign 0x80
sync_current_el_sp0:
  B        k_exphandler_sync_entry //        Synchronous

  .balign 0x80
irq_current_el_sp0:
  B        k_exphandler_irq_entry //        IRQ

  .balign 0x80
fiq_current_el_sp0:
  B        k_exphandler_fiq_entry //        FIQ

  .balign 0x80
serror_current_el_sp0:
  B        k_exphandler_serror_entry //        SError

// ------------------------------------------------------------
// Current EL with SPx
// ------------------------------------------------------------

  .balign 0x80
sync_current_el_spx:
  B        k_exphandler_sync_entry //        Synchronous

  .balign 0x80
irq_current_el_spx:
  B        k_exphandler_irq_entry //        IRQ

  .balign 0x80
fiq_current_el_spx:
  B        k_exphandler_fiq_entry //        FIQ

  .balign 0x80
serror_current_el_spx:
  B        k_exphandler_serror_entry //        SError

// ------------------------------------------------------------
// Lower EL using AArch64
// ------------------------------------------------------------

  .balign 0x80
sync_lower_el_aarch64:
  B        k_exphandler_sync_entry //        Synchronous

  .balign 0x80
irq_lower_el_aarch64:
  B        k_exphandler_irq_entry //        IRQ

  .balign 0x80
fiq_lower_el_aarch64:
  B        k_exphandler_fiq_entry //        FIQ

  .balign 0x80
serror_lower_el_aarch64:
  B        k_exphandler_serror_entry //        SError

// ------------------------------------------------------------
// Lower EL using AArch32
// ------------------------------------------------------------

  .balign 0x80
sync_lower_el_aarch32:
  B        k_exphandler_sync_entry //        Synchronous

  .balign 0x80
irq_lower_el_aarch32:
  B        k_exphandler_irq_entry //        IRQ

  .balign 0x80
fiq_lower_el_aarch32:
  B        k_exphandler_fiq_entry //        FIQ

  .balign 0x80
serror_lower_el_aarch32:
  B        k_exphandler_serror_entry //        SError