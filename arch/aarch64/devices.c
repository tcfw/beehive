#include <kernel/device_interrupt.h>
#include <kernel/devices.h>
#include <kernel/devicetree.h>
#include <kernel/panic.h>
#include <kernel/strings.h>
#include <kernel/tty.h>
#include <devices/pl031.h>

static uint8_t interrupt_size = 0;
static uint8_t gic_version = 0;

const struct arch_device_handler arch_devices[] = {
	{.compat = "arm,pl031",
	 .init = arch_setup_pl031},
};

static void init_interrupt_config()
{
	void *node = devicetree_first_with_property("interrupt-controller");
	if (node == 0)
		panic("no interrupt controller found in DBT");

	char *interrupt_size_c = devicetree_get_property(node, "#interrupt-cells");
	if (interrupt_size_c != 0)
	{
		interrupt_size = (uint8_t)(BIG_ENDIAN_UINT32(*(uint32_t *)interrupt_size_c));
	}

	char *compat = devicetree_get_property(node, "compatible");
	if (strcmp(compat, "arm,gic-v3") == 0)
		gic_version = 3;
	else if (strcmp(compat, "arm,gic-v4") == 0)
		gic_version = 4;
	else
		panicf("unknown interrupt controller %s", compat);
}

static void giv3_get_device_interrupts(device_node_t *info, void *node, char *prop)
{
	uint32_t propLen = devicetree_get_property_len(node, "interrupts");

	for (int n = 0; n < propLen / 3 / sizeof(uint32_t) && n < 5; n++)
		switch (interrupt_size)
		{
		case 3:
			uint32_t int_type = BIG_ENDIAN_UINT32(*(uint32_t *)(prop + (sizeof(uint32_t) * n)));
			uint32_t int_num = BIG_ENDIAN_UINT32(*((uint32_t *)(prop + (sizeof(uint32_t) * n)) + 1));
			uint32_t int_flags = BIG_ENDIAN_UINT32(*((uint32_t *)(prop + (sizeof(uint32_t) * n)) + 2));

			device_interrupt_t *interrupt = &info->interrupt_set.interrupts[n];
			uint32_t xrq_base = 0;

			if (int_type == 0)
			{
				interrupt->type = IRQ_TYPE_SPI;
				xrq_base = IRQ_BASE_NUM_SPI;
			}
			else if (int_type == 1)
			{
				interrupt->type = IRQ_TYPE_PPI;
				xrq_base = IRQ_BASE_NUM_PPI;
			}

			interrupt->xrq = xrq_base + int_num;

			if (int_flags == 1)
				interrupt->trigger = IRQ_TRIGGER_EDGE;
			else if (int_flags == 4)
				interrupt->trigger = IRQ_TRIGGER_LEVEL;

			break;
		default:
			panic("unsupported interrupt cell size for GICv3");
		}
}

static void giv4_get_device_interrupts(device_node_t *info, void *node, void *prop)
{
}

void arch_get_device_interrupts(device_node_t *info, void *node)
{
	if (gic_version == 0 || interrupt_size == 0)
		init_interrupt_config();

	char *prop = devicetree_get_property(node, "interrupts");
	if (prop == 0)
		return;

	switch (gic_version)
	{
	case 3:
		giv3_get_device_interrupts(info, node, prop);
		break;
	case 4:
		giv4_get_device_interrupts(info, node, prop);
		break;
	default:
		panic("unsupported GIC version for DBT interrupt mapping");
	}
}

int arch_should_handle_device(device_node_t *info)
{
	for (int i = 0; i < sizeof(arch_devices) / sizeof(arch_devices[0]); i++)
	{
		if (strcmp(arch_devices[i].compat, info->compatibility) == 0)
		{
			terminal_logf("handling %s internally", info->compatibility);
			return 1;
		}
	}

	return 0;
}

void arch_setup_device(device_node_t *info)
{
	for (int i = 0; i < sizeof(arch_devices) / sizeof(arch_devices[0]); i++)
	{
		if (strcmp(arch_devices[i].compat, info->compatibility) == 0)
		{
			if (arch_devices[i].init != 0)
				arch_devices[i].init(info);

			return;
		}
	}
}