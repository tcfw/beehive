#include <kernel/arch.h>
#include <kernel/tty.h>
#include <kernel/irq.h>
#include <kernel/syscall.h>
#include <kernel/mm.h>
#include <kernel/strings.h>
#include <devicetree.h>
#include <kernel/msgs.h>
#include "stdint.h"

void kernel_main(void)
{
    arch_init();
    terminal_initialize();

    terminal_write(HELLO_HEADER, sizeof(HELLO_HEADER));
    terminal_write(BUILD_INFO, sizeof(BUILD_INFO));
    terminal_write(HELLO_FOOTER, sizeof(HELLO_FOOTER));

    page_alloc_init();
    syscall_init();

    // dumpdevicetree();

    enable_xrq();

    char buf[50];

    void *pg = alloc_page(0);

    ksprintf(&buf[0], "Page allocated: 0x%x", pg);
    terminal_log(buf);

    void *pg1 = alloc_page(0);

    ksprintf(&buf[0], "Page allocated: 0x%x", pg1);
    terminal_log(buf);

    void *pg2 = alloc_page(1);

    ksprintf(&buf[0], "Page allocated: 0x%x", pg2);
    terminal_log(buf);

    for (;;)
        wfi();
}
