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
#include <kernel/clock.h>
#include "stdint.h"

extern void user_init(void);

void kernel_main2(void)
{
    global_clock_init();

    if (cpu_id() != 0)
    {
        core_init();
        arch_init();
    }

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
        thread1->ctx.pc = 0x1000ULL;
        thread1->vm = (vm_table *)page_alloc_s(sizeof(vm_table));

        void *prog = page_alloc(0);
        memcpy(prog, &user_init, 0x10);

        vm_init_table(thread1->vm);
        int r = vm_map_region(thread1->vm, prog, 0x1000ULL, 4095, MEMORY_TYPE_USER);
        if (r < 0)
            terminal_logf("failed to map user region: 0x%x", r);

        get_cls()->currentThread = thread1;
        __asm__ volatile("msr TPIDRRO_EL0, %0" ::"r"(thread1->pid));
        vm_set_table(thread1->vm);
        // switch_to_context(&thread1->ctx);
    }
    else if (cpu_id() == 1)
    {
        struct clocksource_t *cs = clock_first(CS_GLOBAL);
        terminal_logf("Clock Global 0x%x", cs->val(cs));
        uint64_t freq = cs->getFreq(cs);
        uint64_t val = cs->val(cs);
        cs->countNTicks(cs, freq / 2);
        cs->enable(cs);
        cs->enableIRQ(cs, 0);
        __asm__ volatile("ISB");
    }

    wait_task();
}

void kernel_main(void)
{
    registerClocks();
    core_init();
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
