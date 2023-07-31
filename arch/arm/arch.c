#include <kernel/irq.h>
#include <kernel/arch.h>

void arch_init(void)
{
	disable_xrq();
	// init_xrq();
}