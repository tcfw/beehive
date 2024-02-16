#include <kernel/devicetree.h>
#include <kernel/endian.h>
#include <kernel/strings.h>
#include <kernel/tty.h>
#include <kernel/unistd.h>

static uintptr_t dbt_offset = 0x40000000;

static void print_indents(int n)
{
	for (uint8_t i = 0; i < n; i++)
		terminal_putchar('\t');
}

void remaped_devicetreeoffset(uintptr_t offset)
{
	dbt_offset = offset;
}

// TODO(tcfw) remap in VM so can verify magic
void dumpdevicetree()
{
	terminal_logf("DTB Tree::");

	if (dbt_offset != 4)
	{
		terminal_logf("DTB Magic 0x%x", BIG_ENDIAN_UINT32(*(uint32_t *)(dbt_offset)));
	}

	volatile struct fdt_header_t *dtb_header = (struct fdt_header_t *)dbt_offset;

	uint32_t off_dt_struct = BIG_ENDIAN_UINT32(dtb_header->off_dt_struct) + (dbt_offset);
	uint32_t off_dt_strings = BIG_ENDIAN_UINT32(dtb_header->off_dt_strings) + (dbt_offset);

	terminal_logf("DBT Size: 0x%x", BIG_ENDIAN_UINT32(dtb_header->totalsize));
	terminal_logf("DBT DT STRUCT OFF: 0x%x", off_dt_struct);
	terminal_logf("DBT DT STRING OFF: 0x%x", off_dt_strings);

	uint32_t *data = (uint32_t *)off_dt_struct;
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
			terminal_writestring((char *)off_dt_strings + BIG_ENDIAN_UINT32(prop->nameoff));
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

void *devicetree_find_node(char *path)
{
	if (*path == '/')
		path++;

	volatile struct fdt_header_t *dtb_header = (struct fdt_header_t *)dbt_offset;
	uintptr_t off_dt_strings = dbt_offset + BIG_ENDIAN_UINT32(dtb_header->off_dt_strings);
	uint32_t *data = (uint32_t *)(dbt_offset + BIG_ENDIAN_UINT32(dtb_header->off_dt_struct));
	char *keyn;

	while (data < off_dt_strings + BIG_ENDIAN_UINT32(dtb_header->size_dt_struct))
	{
		uint32_t *nt = data++;
		if (*nt == FDT_BEGIN_NODE)
		{
			int nextn = 0;
			char *nextnp = path;
			while (*nextnp != "/" && *nextnp != 0)
			{
				nextnp++;
				nextn++;
			}

			if (memcmp(path, (void *)data, nextn) == 0)
			{
				path = nextnp;
				if (*path == 0)
					return data;

				while (*data)
					data++;

				if ((uint32_t)data % 4 != 0)
					data += 4 - ((uint32_t)data % 4); // ensure aligned back to uint32
			}
			else
			{
				if (*data == 0)
					data++;
				else
				{
					keyn = data;
					while (*keyn)
						keyn++;
					if ((uint32_t)keyn % 4 != 0)
						keyn += 4 - ((uint32_t)keyn % 4); // ensure aligned back to uint32

					data = (uint32_t *)keyn;
				}
			}
		}
		else if (*nt == FDT_PROP)
		{
			struct fdt_prop_t *prop = (struct fdt_prop_t *)data;
			data += sizeof(struct fdt_prop_t) / sizeof(uint32_t);
			uint32_t offset = BIG_ENDIAN_UINT32(prop->len) / sizeof(uint32_t);
			data += BIG_ENDIAN_UINT32(prop->len) / sizeof(uint32_t);
			if (offset * sizeof(uint32_t) != BIG_ENDIAN_UINT32(prop->len))
				data++;
		}
		else if (*nt == FDT_END)
			return 0;
	}

	return 0;
}

char *devicetree_get_property(void *node, char *propkey)
{
	volatile struct fdt_header_t *dtb_header = (struct fdt_header_t *)dbt_offset;
	uintptr_t off_dt_strings = dbt_offset + BIG_ENDIAN_UINT32(dtb_header->off_dt_strings);
	char *data = (char *)node;

	// read past unit name
	if (*data == 0)
		data += 4;
	else
		while (*data)
			data++;

	// read past padding, if any
	if ((uint32_t)data % 4 != 0)
		data += 4 - ((uint32_t)data % 4); // ensure aligned back to uint32

	while (*((uint32_t *)data) == FDT_PROP)
	{
		data += sizeof(uint32_t);
		struct fdt_prop_t *prop = (struct fdt_prop_t *)data;
		data += sizeof(struct fdt_prop_t);
		char *name = (char *)(off_dt_strings + (uintptr_t)BIG_ENDIAN_UINT32(prop->nameoff));
		if (strcmp(name, propkey) == 0)
			return (char *)data;

		data += BIG_ENDIAN_UINT32(prop->len);
		if ((uint32_t)data % 4 != 0)
			data += 4 - ((uint32_t)data % 4); // ensure aligned back to uint32
	}

	return 0;
}

char *devicetree_get_root_property(char *propkey)
{
	volatile struct fdt_header_t *dtb_header = (struct fdt_header_t *)dbt_offset;
	uint32_t *root = (uint32_t *)(dbt_offset + BIG_ENDIAN_UINT32(dtb_header->off_dt_struct));

	return devicetree_get_property(root + 1, propkey);
}

uint32_t devicetree_count_nodes_with_prop(char *propkey, char *cmpdata, size_t size)
{
	uint32_t count = 0;

	volatile struct fdt_header_t *dtb_header = (struct fdt_header_t *)dbt_offset;
	uintptr_t off_dt_strings = dbt_offset + BIG_ENDIAN_UINT32(dtb_header->off_dt_strings);
	uint32_t *data = (uint32_t *)(dbt_offset + BIG_ENDIAN_UINT32(dtb_header->off_dt_struct));
	char *keyn;

	while (data < off_dt_strings + BIG_ENDIAN_UINT32(dtb_header->size_dt_struct))
	{
		switch (*data++)
		{
		case FDT_BEGIN_NODE:;
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

			char *nodePropKey = (char *)(off_dt_strings + BIG_ENDIAN_UINT32(prop->nameoff));
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

void *devicetree_first_with_device_type(char *type)
{
}