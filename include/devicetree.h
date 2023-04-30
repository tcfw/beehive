#ifndef _DEVICETREE_H
#define _DEVICETREE_H

#include "stdint.h"
#include <kernel/endian.h>

#define FDT_MAGIC BIG_ENDIAN_UINT32(0xd00dfeed)

#define FDT_BEGIN_NODE BIG_ENDIAN_UINT32(0x00000001)
#define FDT_END_NODE BIG_ENDIAN_UINT32(0x00000002)
#define FDT_PROP BIG_ENDIAN_UINT32(0x00000003)
#define FDT_NOP BIG_ENDIAN_UINT32(0x00000004)
#define FDT_END BIG_ENDIAN_UINT32(0x00000009)

struct fdt_header_t
{
	// uint32_t magic;
	uint32_t totalsize;
	uint32_t off_dt_struct;
	uint32_t off_dt_strings;
	uint32_t off_mem_rsvmap;
	uint32_t version;
	uint32_t last_comp_version;
	uint32_t boot_cpuid_phys;
	uint32_t size_dt_strings;
	uint32_t size_dt_struct;
};

struct fdt_reserve_entry_t
{
	uint64_t address;
	uint64_t size;
};

struct fdt_prop_t
{
	uint32_t len;
	uint32_t nameoff;
};

void dumpdevicetree();

#endif