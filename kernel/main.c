#include <kernel/arch.h>
#include <kernel/buddy.h>
#include <kernel/clock.h>
#include <kernel/cls.h>
#include <kernel/debug.h>
#include <kernel/devices.h>
#include <kernel/devicetree.h>
#include <kernel/elf.h>
#include <kernel/futex.h>
#include <kernel/initproc.h>
#include <kernel/irq.h>
#include <kernel/mm.h>
#include <kernel/modules.h>
#include <kernel/msgs.h>
#include <kernel/queue.h>
#include <kernel/regions.h>
#include <kernel/stdint.h>
#include <kernel/strings.h>
#include <kernel/syscall.h>
#include <kernel/thread.h>
#include <kernel/tty.h>
#include <kernel/vm.h>
#include <tests/tests.h>

void kernel_main2(void);
extern void user_init(void);

static void thread_test(void *data)
{
    struct clocksource_t *cs = clock_first(CS_GLOBAL);
    timespec_t now;

    terminal_logf("data was: %s", (char *)data);

    while (1)
    {
        timespec_from_cs(cs, &now);
        terminal_logf("kthread ellapsed: 0x%X 0x%X", now.seconds, now.nanoseconds);

        timespec_t ss = {.seconds = 60};
        sleep_kthread(&ss, NULL);
    }
}

static void init_mon(void *data)
{
    int i = 0;
    timespec_t ss = {.seconds = 2, .nanoseconds = 250000000};
    sleep_kthread(&ss, NULL);
    ss.seconds = 0;
    terminal_printf("\r\n\033c");

    while (1)
    {
        i++;
        if (i % 20 == 0)
            terminal_printf("\r\n\033c");

        terminal_printf("\r\n\033[;H");
        terminal_logf("\r\nProc\tCore\tPC\t\tTime\t\t\tDeadline\t\tState\r\n");

        thread_list_entry_t *this;

        list_head_for_each(this, get_threads())
        {
            if (this->thread->process->pid == 1 && this->thread->process->state == ZOMBIE)
            {
                terminal_writestring("init pid a zombie");
                goto fin;
            }

            if (this->thread->process->pid == 0)
                terminal_writestring(this->thread->name);
            else
                terminal_printf("[%d:%d]", this->thread->process->pid, this->thread->tid);

            if (this->thread == current)
                terminal_writestring("*");
            else
                terminal_writestring(" ");

            terminal_printf("\t%d\t0x%x\t%X\t%X\t%d", this->thread->running_core, this->thread->ctx.pc, this->thread->timing.total_execution, this->thread->sched_entity.deadline, this->thread->state);

            switch (this->thread->state)
            {
            case THREAD_RUNNING:
                terminal_writestring("\tRunning");
                break;
            case THREAD_SLEEPING:
                terminal_writestring("\tSleeping");
                if (this->thread->wc != NULL)
                {
                    if (this->thread->wc->type == WAIT)
                    {
                        struct thread_wait_cond_futex *wc = (struct thread_wait_cond_futex *)this->thread->wc;
                        if (wc->timeout != NULL)
                            terminal_writestring(" with timeout");
                        terminal_printf(" for futex=0x%X", wc->queue->key.both);
                    }
                    else if (this->thread->wc->type == SLEEP)
                    {
                        struct thread_wait_cond_sleep *wc = (struct thread_wait_cond_sleep *)this->thread->wc;
                        terminal_printf(" until %d.%d", wc->timer.seconds, wc->timer.nanoseconds);
                    }
                    else
                        terminal_printf(" WC=0x%x", this->thread->wc->type);
                }
                else
                    terminal_writestring(" NWC!");

                break;
            case THREAD_UNINT_SLEEPING:
                terminal_writestring("\tUint sleeping");
                break;
            case THREAD_STOPPED:
                terminal_writestring("\tStopped");
                break;
            case THREAD_DEAD:
                terminal_printf("\tDead(%d)", this->thread->process->exitCode);
                break;
            }

            terminal_writestring("\r\n");
        }

        ss.seconds = 2;
        sleep_kthread(&ss, NULL);
    }
fin:
    terminal_writestring("stopped [mon]");
    mark_zombie_thread(current);
    syscall0(SYSCALL_SCHED_YIELD);
}

static void setup_init_threads(void)
{
    // thread_t *kthread1 = create_kthread(&thread_test, "[hello world]", (void *)"test");
    // sched_append_pending(kthread1);

    // thread_t *kthread2 = create_kthread(&init_mon, "[mon]", NULL);
    // sched_append_pending(kthread2);

    terminal_log("Loading init proc...");
    int ret = load_initproc();
    if (ret < 0)
        terminal_logf("Failed to load init proc %d", ret);
    else
        terminal_log("Loaded init proc");
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

    global_clock_init();
    page_alloc_init();
    slub_alloc_init();
    vm_init();
    init_cls();
    sched_init();
    syscall_init();
    queues_init();
    futex_init();
    mod_init();

    init_kthread_proc();
    setup_init_threads();

    if (RUN_SELF_TESTS == 1)
    {
        sched_local_init();
        run_self_tests();
        return;
    }

    wake_cores();
    kernel_main2();
}

void kernel_main2(void) __attribute__((kernel))
{
    static uint32_t booted;
    static uint32_t vm_ready;

    if (cpu_id() != 0)
    {
        core_init();
        arch_init();
    }

    vm_set_kernel();
    vm_enable();
    remaped_devicetreeoffset(DEVICE_DESCRIPTOR_REGION);
    terminal_set_bar(DEVICE_REGION);
    sched_local_init();

    enable_xrq();
    disable_irq();

    __atomic_add_fetch(&booted, 1, __ATOMIC_ACQ_REL);
    terminal_logf("Booted core 0x%x", get_cls()->id);

    if (cpu_id() == 0)
    {
        terminal_logf("Waiting for other cores to boot...");

        unsigned int cpuN = devicetree_count_dev_type("cpu");
        while (cpuN != __atomic_load_n(&booted, __ATOMIC_CONSUME))
        {
        }

        vm_init_post_enable();
        discover_devices();

        __atomic_store_n(&vm_ready, 1, __ATOMIC_RELAXED);
    }
    else
        while (__atomic_load_n(&vm_ready, __ATOMIC_RELAXED) == 0)
        {
        }

    enable_irq();
    schedule_start();
}