#include <kernel/arch.h>
#include <kernel/tty.h>
#include <kernel/strings.h>
#include <kernel/irq.h>
#include "pe.h"
#include <strings.h>

#include "stdint.h"

int syscall4(uint32_t arg1)
{
    uint32_t ret;
    __asm__ volatile("mov x0, %0" ::"r"(arg1));
    __asm__ volatile("mov x1, #0");
    __asm__ volatile("mov x2, #0");
    __asm__ volatile("mov x3, #0");
    __asm__ volatile("svc #4");
    __asm__ volatile("mov %0, x0"
                     : "=r"(ret));

    return ret;
}

void kernel_main(void)
{
    arch_init();

    terminal_initialize();
    terminal_writestring("Beehive OS\n");

    char buf[50];
    ksprintf(&buf[0], "Current EL: %x\n", currentEL());

    terminal_writestring(buf);

    enable_xrq();

    uint32_t ret = syscall4(4);
    if (ret == 3)
    {
        terminal_writestring("\nsyscall returned value was 3!!!\n");
    }

    enable_xrq_n(33); // SPI (32+)1

    for (;;)
    {
        wfi();
    }
}
