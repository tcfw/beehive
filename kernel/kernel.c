#include <kernel/arch.h>
#include <kernel/tty.h>
#include <kernel/irq.h>
#include <kernel/syscall.h>
#include <kernel/mm.h>
#include <kernel/vm.h>
#include <kernel/strings.h>
#include <devicetree.h>
#include <kernel/msgs.h>
#include <kernel/regions.h>
#include <kernel/cls.h>
#include <kernel/thread.h>
#include "stdint.h"

void kernel_main2(void)
{
    arch_init();
    enable_xrq();

    vm_set_kernel();
    vm_enable();
    terminal_set_bar(DEVICE_REGION);
    remaped_devicetreeoffset(ram_max() + 4);

    terminal_logf("Booted core 0x%x", get_cls()->id);

    if (cpu_id() == 0)
    {
        // dumpdevicetree();
        thread_t *thread1 = (thread_t *)page_alloc_s(sizeof(thread_t));
        init_thread(thread1);
        thread1->pid = 1;
        thread1->ctx.pc = 0x1000;
        thread1->vm = (vm_table *)page_alloc_s(sizeof(vm_table));
        thread1->ctx.regs[0] = 0;
        thread1->ctx.regs[1] = 1;
        thread1->ctx.regs[2] = 2;
        thread1->ctx.regs[3] = 3;
        thread1->ctx.regs[4] = 4;
        thread1->ctx.regs[5] = 5;
        thread1->ctx.regs[6] = 6;
        thread1->ctx.regs[7] = 7;
        thread1->ctx.regs[8] = 8;
        thread1->ctx.regs[9] = 9;
        vm_init_table(thread1->vm);
        get_cls()->currentThread = thread1;
        __asm__ volatile("msr TPIDRRO_EL0, %0" ::"r"(thread1->pid));
        vm_set_table(thread1->vm);
        switch_to_context(&thread1->ctx);
    }

    wait_task();
}

void kernel_main(void)
{
    arch_init();
    terminal_initialize();

    uint32_t coreCount = devicetree_count_dev_type("cpu");

    terminal_write(HELLO_HEADER, sizeof(HELLO_HEADER));
    terminal_write(BUILD_INFO, sizeof(BUILD_INFO));
    terminal_write(HELLO_FOOTER, sizeof(HELLO_FOOTER));
    terminal_logf("CPU Brand: 0x%x", cpu_brand());
    terminal_logf("CPU Count: 0x%x", coreCount);

    page_alloc_init();
    vm_init();
    syscall_init();
    init_cls(coreCount);

    wake_cores();
    kernel_main2();
}
