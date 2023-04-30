#include "devicetree.h"
#include <kernel/endian.h>
#include <kernel/strings.h>
#include <kernel/tty.h>

static void print_indents(int n)
{
	for (uint8_t i = 0; i < n; i++)
		terminal_putchar('\t');
}

// TODO(tcfw) remap in VM so can verify magic
void dumpdevicetree()
{
	char buf[1000];

	volatile struct fdt_header_t *dtb_header = (struct fdt_header_t *)4;

	uint32_t off_dt_struct = BIG_ENDIAN_UINT32(dtb_header->off_dt_struct);
	uint32_t off_dt_strings = BIG_ENDIAN_UINT32(dtb_header->off_dt_strings);

	terminal_writestring("DTB Tree::\n");

	ksprintf(&buf[0], "DBT Size: 0x%x\n", BIG_ENDIAN_UINT32(dtb_header->totalsize));
	terminal_writestring(buf);

	ksprintf(&buf[0], "DBT DT STRUCT OFF: 0x%x\n", off_dt_struct);
	terminal_writestring(buf);

	ksprintf(&buf[0], "DBT DT STRING OFF: 0x%x\n", off_dt_strings);
	terminal_writestring(buf);

	uint64_t *data = off_dt_struct;
	struct fdt_prop_t *prop;
	char *keyn;
	unsigned int ndepth = 0;

	while (data < off_dt_strings + BIG_ENDIAN_UINT32(dtb_header->size_dt_struct))
	{
		switch (*data++)
		{
		case FDT_BEGIN_NODE:
			// node key is null-term string
			print_indents(ndepth++);
			terminal_writestring("node ");

			keyn = data;
			while (*keyn)
				terminal_putchar(*keyn++);
			if ((uintptr_t)keyn % 4 != 0)
			{
				keyn += 4 - ((uint8_t)keyn % 4); // ensure aligned back to uint32
			}
			data = (uint32_t *)keyn;
			terminal_writestring(" {\n");
			break;
		case FDT_END_NODE:
			ndepth--;
			print_indents(ndepth);
			terminal_writestring("}\n");
			break;
		case FDT_PROP:
			prop = data; // data already moved halfway through sizeof(prop)
			data += 2;
			print_indents(ndepth);
			terminal_writestring(off_dt_strings + BIG_ENDIAN_UINT32(prop->nameoff));
			terminal_writestring(" => ");
			keyn = data;
			uint32_t len = BIG_ENDIAN_UINT32(prop->len);
			char *end = (void *)data + len;
			if (len != 0)
			{
				while (keyn < end)
					terminal_putchar(*keyn++);

				if ((uintptr_t)keyn % 4 != 0)
				{
					keyn += 4 - ((uint8_t)keyn % 4); // ensure aligned back to uint32
				}
			}

			data = (uint32_t *)keyn;
			terminal_writestring("\n");
			break;
		case FDT_END:
			return;
			break;
		case FDT_NOP:
		default:
			break;
		}
	}
}
