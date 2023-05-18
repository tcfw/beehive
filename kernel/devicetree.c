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
	volatile struct fdt_header_t *dtb_header = (struct fdt_header_t *)4;

	uint32_t off_dt_struct = BIG_ENDIAN_UINT32(dtb_header->off_dt_struct);
	uint32_t off_dt_strings = BIG_ENDIAN_UINT32(dtb_header->off_dt_strings);

	terminal_log("DTB Tree::");
	terminal_logf("DBT Size: 0x%x", BIG_ENDIAN_UINT32(dtb_header->totalsize));
	terminal_logf("DBT DT STRUCT OFF: 0x%x", off_dt_struct);
	terminal_logf("DBT DT STRING OFF: 0x%x", off_dt_strings);

	uint32_t *data = off_dt_struct;
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

			keyn = data;
			if (*keyn == 0)
				terminal_putchar('/');

			while (*keyn)
				terminal_putchar(*keyn++);

			if ((uint32_t)keyn % 4 != 0)
				keyn += 4 - ((uint32_t)keyn % 4); // ensure aligned back to uint32

			data = (uint32_t *)keyn;
			terminal_writestring(" {\n");
			break;
		case FDT_END_NODE:
			ndepth--;
			print_indents(ndepth);
			terminal_writestring("};\n");
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

				if ((uint32_t)keyn % 4 != 0)
					keyn += 4 - ((uint32_t)keyn % 4); // ensure aligned back to uint32
			}

			data = (uint32_t *)keyn;
			terminal_writestring(";\n");
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

uint32_t devicetree_count_nodes_with_prop(char *propkey, char *cmpdata, size_t size)
{
	uint32_t count = 0;

	volatile struct fdt_header_t *dtb_header = (struct fdt_header_t *)4;
	uint32_t off_dt_strings = BIG_ENDIAN_UINT32(dtb_header->off_dt_strings);
	uint32_t *data = BIG_ENDIAN_UINT32(dtb_header->off_dt_struct);
	char *keyn;

	while (data < off_dt_strings + BIG_ENDIAN_UINT32(dtb_header->size_dt_struct))
	{
		switch (*data++)
		{
		case FDT_BEGIN_NODE:
			keyn = data;
			while (*keyn)
				keyn++;
			if ((uint32_t)keyn % 4 != 0)
				keyn += 4 - ((uint32_t)keyn % 4); // ensure aligned back to uint32

			data = (uint32_t *)keyn;
			break;
		case FDT_PROP:;
			uint32_t keymatch = 0;
			struct fdt_prop_t *prop = data; // data already moved halfway through sizeof(prop)
			data += 2;

			char *nodePropKey = off_dt_strings + BIG_ENDIAN_UINT32(prop->nameoff);
			if (strcmp(nodePropKey, propkey) == 0)
				keymatch = 1;

			keyn = data;
			uint32_t len = BIG_ENDIAN_UINT32(prop->len);
			if (len == size)
				keymatch = 2;

			char *end = (void *)data + len;
			if (len != 0)
			{
				uint32_t offs = 0;
				while (keyn < end)
				{
					if (keymatch == 2 && !(*keyn == cmpdata[offs]))
						keymatch = 0;
					offs++;
					keyn++;
				}

				if ((uint32_t)keyn % 4 != 0)
					keyn += 4 - ((uint32_t)keyn % 4); // ensure aligned back to uint32
			}

			data = (uint32_t *)keyn;
			if (keymatch == 2)
				count++;
			keymatch = 0;
			break;
		case FDT_END:
			return count;
		case FDT_END_NODE:
		case FDT_NOP:
		default:
			break;
		}
	}

	return count;
}

uint32_t devicetree_count_dev_type(char *type)
{
	return devicetree_count_nodes_with_prop(FDT_DEVICE_TYPE_PROP, type, strlen(type) + 1);
}