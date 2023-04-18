#include <kernel/arch.h>
#include <kernel/tty.h>
#include <kernel/irq.h>
#include <kernel/syscall.h>
#include "stdint.h"

#define HELLO_MSG "\nBeehive OS\n\n"

void kernel_main(void)
{
    arch_init();

    terminal_initialize();
    terminal_writestring(HELLO_MSG);

    syscall_init();
    enable_xrq();

    for (;;)
        wfi();
}
