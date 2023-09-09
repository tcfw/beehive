#include <devicetree.h>
#include <kernel/arch.h>
#include <kernel/clock.h>
#include <kernel/cls.h>
#include <kernel/init.h>
#include <kernel/irq.h>
#include <kernel/mm.h>
#include <kernel/msgs.h>
#include <kernel/regions.h>
#include <kernel/strings.h>
#include <kernel/syscall.h>
#include <kernel/thread.h>
#include <kernel/tty.h>
#include <kernel/vm.h>
#include "stdint.h"

extern void user_init(void);

static void mod_init(void);

void kernel_main2(void)
{
    global_clock_init();

    if (cpu_id() != 0)
    {
        core_init();
        arch_init();
    }

    vm_set_kernel();
    vm_enable();
    sched_local_init();

    enable_xrq();
    terminal_set_bar(DEVICE_REGION);
    remaped_devicetreeoffset(ram_max() + 4);

    terminal_logf("Booted core 0x%x", get_cls()->id);

    struct clocksource_t *cs = clock_first(CS_GLOBAL);

    if (cpu_id() == 0)
    {

        thread_t *thread1 = sched_get_pending(sched_affinity(cpu_id()));
        if (thread1 != NULL)
        {
            set_current_thread(thread1);
            arch_thread_prep_switch(thread1);
            vm_set_table(thread1->vm_table);
            switch_to_context(&thread1->ctx);
        }
    }
    else if (cpu_id() == 1)
    {
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

void mod_init(void)
{
    terminal_log("Loading modules...");

    for (init_func_ptr_t *cb =
             ({
                 extern init_func_ptr_t __start_init;
                 &__start_init;
             });
         cb !=
         ({
             extern init_func_ptr_t __stop_init;
             &__stop_init;
         });
         ++cb)
    {
        cb->callback();
    }

    terminal_log("Loaded modules");
}

static void setup_init_thread(void)
{
    thread_t *init = (thread_t *)page_alloc_s(sizeof(thread_t));
    init_thread(init);
    strcpy(init->cmd, "init");
    init->pid = 1;
    init->uid = 1;
    init->euid = 1;
    init->gid = 1;
    init->egid = 1;
    init->ctx.pc = 0x1000ULL;

    void *prog = page_alloc_s(0x1c);
    memcpy(prog, &user_init, 0x1c);

    int r = vm_map_region(init->vm_table, prog, 0x1000ULL, 4095, MEMORY_TYPE_USER);
    if (r < 0)
        terminal_logf("failed to map user region: 0x%x", r);

    sched_append_pending(init);
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
    syscall_init();
    init_cls(coreCount);
    vm_init();
    sched_init();

    mod_init();

    setup_init_thread();

    wake_cores();
    kernel_main2();
}
