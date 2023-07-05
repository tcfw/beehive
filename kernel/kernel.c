#include <kernel/arch.h>
#include <kernel/tty.h>
#include <kernel/irq.h>
#include <kernel/syscall.h>
#include <kernel/mm.h>
#include <kernel/vm.h>
#include <kernel/strings.h>
#include <devicetree.h>
#include <kernel/msgs.h>
#include "stdint.h"

void kernel_main2(void)
{
    arch_init();
    enable_xrq();

    vm_set_kernel();
    vm_enable();

    terminal_logf("Booted core 0x%x", cpu_id());

    if (cpu_id() == 0)
    {
        remaped_devicetreeoffset(RAM_MAX + 4);
        dumpdevicetree();
    }

    for (;;)
        wfi();
}

void kernel_main(void)
{
    arch_init();
    terminal_initialize();

    terminal_write(HELLO_HEADER, sizeof(HELLO_HEADER));
    terminal_write(BUILD_INFO, sizeof(BUILD_INFO));
    terminal_write(HELLO_FOOTER, sizeof(HELLO_FOOTER));
    terminal_logf("CPU Brand: 0x%x", cpu_brand());
    terminal_logf("CPU Count: 0x%x", devicetree_count_dev_type("cpu"));

    page_alloc_init();
    vm_init();
    syscall_init();

    wake_cores();
    kernel_main2();
}
