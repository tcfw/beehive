#include <kernel/stdint.h>
#include <kernel/devicetree.h>
#include <kernel/arch.h>
#include <kernel/buddy.h>
#include <kernel/clock.h>
#include <kernel/cls.h>
#include <kernel/devices.h>
#include <kernel/irq.h>
#include <kernel/mm.h>
#include <kernel/modules.h>
#include <kernel/msgs.h>
#include <kernel/queue.h>
#include <kernel/regions.h>
#include <kernel/strings.h>
#include <kernel/syscall.h>
#include <kernel/thread.h>
#include <kernel/tty.h>
#include <kernel/vm.h>
#include <tests/tests.h>

void kernel_main2(void);
extern void user_init(void);
static void thread_test(void *data);

static void setup_init_threads(void)
{
    // thread_t *init = (thread_t *)page_alloc_s(sizeof(thread_t));
    // init_thread(init);
    // strcpy(init->cmd, "init");
    // init->pid = 1;
    // init->uid = 0;
    // init->euid = 0;
    // init->gid = 0;
    // init->egid = 0;
    // init->ctx.pc = 0x1000ULL;

    // void *prog = page_alloc_s(0x1c);
    // memcpy(prog, &user_init, 0x1c);

    // int r = vm_map_region(init->vm_table, (uintptr_t)prog, 0x1000ULL, 4095, MEMORY_TYPE_USER);
    // if (r < 0)
    //     terminal_logf("failed to map user region: 0x%x", r);

    // sched_append_pending(init);

    thread_t *kthread1 = create_kthread(&thread_test, "[hello world]", (void *)"test");
    sched_append_pending(kthread1);
}

static void thread_test(void *data)
{
    struct clocksource_t *cs = clock_first(CS_GLOBAL);
    timespec_t now;

    terminal_logf("data was: %s", (char *)data);

    while (1)
    {
        timespec_from_cs(cs, &now);
        terminal_logf("kthread ellapsed: %x %x", now.seconds, now.nanoseconds);

        timespec_t ss = {.seconds = 10};
        timespec_t rem;
        sleep_kthread(&ss, &rem);
    }
}

void kernel_main(void)
{
    registerClocks();
    core_init();
    arch_init();
    terminal_initialize();

    terminal_write(HELLO_HEADER, sizeof(HELLO_HEADER));
    terminal_write(BUILD_INFO, sizeof(BUILD_INFO));
    terminal_write(HELLO_FOOTER, sizeof(HELLO_FOOTER));

    // dumpdevicetree();

    page_alloc_init();
    slub_alloc_init();
    vm_init();
    init_cls();
    sched_init();
    syscall_init();
    queues_init();
    mod_init();

    if (RUN_SELF_TESTS == 1)
    {
        sched_local_init();
        run_self_tests();
        return;
    }

    setup_init_threads();

    wake_cores();
    kernel_main2();
}

void kernel_main2(void)
{
    static uint32_t booted;
    static uint32_t vm_ready;

    global_clock_init();

    if (cpu_id() != 0)
    {
        core_init();
        arch_init();
    }

    vm_set_kernel();
    vm_enable();
    remaped_devicetreeoffset(DEVICE_DESCRIPTOR_REGION);
    sched_local_init();

    enable_xrq();
    disable_irq();
    terminal_set_bar(DEVICE_REGION);

    __atomic_add_fetch(&booted, 1, __ATOMIC_ACQ_REL);
    terminal_logf("Booted core 0x%x", get_cls()->id);

    if (cpu_id() == 0)
    {
        unsigned int cpuN = devicetree_count_dev_type("cpu");
        while (cpuN != __atomic_load_n(&booted, __ATOMIC_CONSUME))
        {
        }

        vm_init_post_enable();

        __atomic_store_n(&vm_ready, 1, __ATOMIC_RELAXED);
    }
    else
        while (__atomic_load_n(&vm_ready, __ATOMIC_RELAXED) == 0)
        {
        }

    enable_irq();
    schedule_start();
}