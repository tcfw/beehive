#include <kernel/cls.h>
#include <kernel/mm.h>
#include <kernel/arch.h>
#include <kernel/devicetree.h>
#include <kernel/stdint.h>

static cls_t *cls;
static uint8_t max_cls;

void init_cls()
{
	uint32_t n = devicetree_count_dev_type("cpu");

	max_cls = n;

	cls = (cls_t *)page_alloc_s(n * sizeof(cls_t));

	for (uint32_t i = 0; i < n; i++)
	{
		cls_t *ccls = (cls_t *)(cls + i);
		ccls->id = (uint64_t)i;
	}
}

cls_t *get_cls(void)
{
	uint32_t id = cpu_id();
	return (cls_t *)(cls + id);
}

cls_t *get_core_cls(uint8_t n)
{
	if (n > max_cls)
		return 0;

	return (cls_t *)(cls + n);
}

void set_current_thread(thread_t *thread)
{
	cls_t *cls=get_cls();

	cls->rq.current_thread = thread;
	__asm__ volatile("MSR SPSR_EL1, %0" :: "r"(thread->ctx.spsr));
	__asm__ volatile("MSR ELR_EL1, %0" :: "r"(thread->ctx.pc));
}