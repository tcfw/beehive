#ifndef SYSINFO_VERSION
#define SYSINFO_VERSION " "
#endif

#ifndef SYSINFO_ARCH
#define SYSINFO_ARCH " "
#endif

#ifndef _KERNEL_SYSINFO_H
#define _KERNEL_SYSINFO_H

struct kname
{
	char name[64];
	char version[64];
	char arch[64];
};

struct sysinfo
{
	uint64_t ncpu;
	uint32_t page_size;
};

#endif