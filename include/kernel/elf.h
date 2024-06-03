#ifndef _KERNEL_ELF_H
#define _KERNEL_ELF_H

#include <kernel/stdint.h>
#include <kernel/thread.h>

#define ELF_EI_OFFSET_MAG0 0	   //	File identification
#define ELF_EI_OFFSET_MAG1 1	   //	File identification
#define ELF_EI_OFFSET_MAG2 2	   //	File identification
#define ELF_EI_OFFSET_MAG3 3	   //	File identification
#define ELF_EI_OFFSET_CLASS 4	   //	File class
#define ELF_EI_OFFSET_DATA 5	   //	Data encoding
#define ELF_EI_OFFSET_VERSION 6	   //	File version
#define ELF_EI_OFFSET_OSABI 7	   //	Operating system/ABI identification
#define ELF_EI_OFFSET_ABIVERSION 8 //	ABI version
#define ELF_EI_OFFSET_PAD 9		   //	Start of padding bytes
#define ELF_NIDENT_SIZE 16

#define ELF_MAG0 0x7f
#define ELF_MAG1 'E'
#define ELF_MAG2 'L'
#define ELF_MAG3 'F'

#define ELF_CLASS_NONE 0 // None
#define ELF_CLASS_32 1	 // 32bit
#define ELF_CLASS_64 2	 // 64bit

#define ELF_DATA_NONE 0 // No set encoding
#define ELF_DATA_LE 1	// Little endianness
#define ELF_DATA_BE 2	// Big endianness

#define ELF_ET_NONE 0		 //	No file type
#define ELF_ET_REL 1		 //	Relocatable file
#define ELF_ET_EXEC 2		 //	Executable file
#define ELF_ET_DYN 3		 //	Shared object file
#define ELF_ET_CORE 4		 //	Core file
#define ELF_ET_LOOS 0xfe00	 //	Operating system-specific
#define ELF_ET_HIOS 0xfeff	 //	Operating system-specific
#define ELF_ET_LOPROC 0xff00 //	Processor-specific
#define ELF_ET_HIPROC 0xffff //	Processor-specific

#define ELF_OSABI_NONE 0	 //	No extensions or unspecified
#define ELF_OSABI_HPUX 1	 //	Hewlett-Packard HP-UX
#define ELF_OSABI_NETBSD 2	 //	NetBSD
#define ELF_OSABI_LINUX 3	 //	Linux
#define ELF_OSABI_SOLARIS 6	 //	Sun Solaris
#define ELF_OSABI_AIX 7		 //	AIX
#define ELF_OSABI_IRIX 8	 //	IRIX
#define ELF_OSABI_FREEBSD 9	 //	FreeBSD
#define ELF_OSABI_TRU64 10	 //	Compaq TRU64 UNIX
#define ELF_OSABI_MODESTO 11 //	Novell Modesto
#define ELF_OSABI_OPENBSD 12 //	Open BSD
#define ELF_OSABI_OPENVMS 13 //	Open VMS
#define ELF_OSABI_NSK 14	 //	Hewlett-Packard Non-Stop Kernel
#define ELF_OSABI_BEEHIVE 19 //	Beehive (us!)

/*
ELF fields:
	e_ident[EI_NIDENT]: ELF identification
	e_type: object file type
	e_machine: machine type
	e_version: ELF version
	e_entry: Entry point
	e_phoff: program header table offset
	e_shoff: section header table offset
	e_flags: processor-specific flags
	e_ehsize: ELF header size
	e_phentsize: program header table entry size
	e_phnum: program header entry count
	e_shentsize: section header entry size
	e_shnum: section header entry count
	e_shstrndx: section header table index
*/

struct elf_hdr_32_t
{
	unsigned char e_ident[ELF_NIDENT_SIZE];
	uint16_t e_type;
	uint16_t e_machine;
	uint32_t e_version;
	uint32_t e_entry;
	uint32_t e_phoff;
	uint32_t e_shoff;
	uint32_t e_flags;
	uint16_t e_ehsize;
	uint16_t e_phentsize;
	uint16_t e_phnum;
	uint16_t e_shentsize;
	uint16_t e_shnum;
	uint16_t e_shstrndx;
};

struct elf_hdr_64_t
{
	unsigned char e_ident[ELF_NIDENT_SIZE];
	uint16_t e_type;
	uint16_t e_machine;
	uint32_t e_version;
	uint64_t e_entry;
	uint64_t e_phoff;
	uint64_t e_shoff;
	uint32_t e_flags;
	uint16_t e_ehsize;
	uint16_t e_phentsize;
	uint16_t e_phnum;
	uint16_t e_shentsize;
	uint16_t e_shnum;
	uint16_t e_shstrndx;
};

#define ELF_PT_NULL 0
#define ELF_PT_LOAD 1
#define ELF_PT_DYNAMIC 2
#define ELF_PT_INTERP 3
#define ELF_PT_NOTE 4
#define ELF_PT_SHLIB 5
#define ELF_PT_PHDR 6
#define ELF_PT_TLS 7		   /* Thread local storage segment */
#define ELF_PT_LOOS 0x60000000 /* OS-specific */
#define ELF_PT_HIOS 0x6fffffff /* OS-specific */
#define ELF_PT_LOPROC 0x70000000
#define ELF_PT_HIPROC 0x7fffffff
#define ELF_PT_GNU_EH_FRAME (PT_LOOS + 0x474e550)
#define ELF_PT_GNU_STACK (PT_LOOS + 0x474e551)
#define ELF_PT_GNU_RELRO (PT_LOOS + 0x474e552)
#define ELF_PT_GNU_PROPERTY (PT_LOOS + 0x474e553)

/* These constants define the permissions on sections in the program
   header, p_flags. */
#define PF_R 0x4
#define PF_W 0x2
#define PF_X 0x1

struct elf_phdr_32_t
{
	uint32_t p_type;
	uint32_t p_offset;
	uint32_t p_vaddr;
	uint32_t p_paddr;
	uint32_t p_filesz;
	uint32_t p_memsz;
	uint32_t p_flags;
	uint32_t p_align;
};

struct elf_phdr_64_t
{
	uint32_t p_type;
	uint32_t p_flags;
	uint64_t p_offset;
	uint64_t p_vaddr;
	uint64_t p_paddr;
	uint64_t p_filesz;
	uint64_t p_memsz;
	uint64_t p_align;
};

/* sh_type */
#define ELF_SHT_NULL 0
#define ELF_SHT_PROGBITS 1
#define ELF_SHT_SYMTAB 2
#define ELF_SHT_STRTAB 3
#define ELF_SHT_RELA 4
#define ELF_SHT_HASH 5
#define ELF_SHT_DYNAMIC 6
#define ELF_SHT_NOTE 7
#define ELF_SHT_NOBITS 8
#define ELF_SHT_REL 9
#define ELF_SHT_SHLIB 10
#define ELF_SHT_DYNSYM 11
#define ELF_SHT_NUM 12
#define ELF_SHT_LOPROC 0x70000000
#define ELF_SHT_HIPROC 0x7fffffff
#define ELF_SHT_LOUSER 0x80000000
#define ELF_SHT_HIUSER 0xffffffff

/* sh_flags */
#define ELF_SHF_WRITE 0x1
#define ELF_SHF_ALLOC 0x2
#define ELF_SHF_EXECINSTR 0x4
#define ELF_SHF_RELA_LIVEPATCH 0x00100000
#define ELF_SHF_RO_AFTER_INIT 0x00200000
#define ELF_SHF_MASKPROC 0xf0000000

/* special section indexes */
#define ELF_SHN_UNDEF 0
#define ELF_SHN_LORESERVE 0xff00
#define ELF_SHN_LOPROC 0xff00
#define ELF_SHN_HIPROC 0xff1f
#define ELF_SHN_LIVEPATCH 0xff20
#define ELF_SHN_ABS 0xfff1
#define ELF_SHN_COMMON 0xfff2
#define ELF_SHN_HIRESERVE 0xffff

struct elf_shdr_32
{
	uint32_t sh_name;
	uint32_t sh_type;
	uint32_t sh_flags;
	uint32_t sh_addr;
	uint32_t sh_offset;
	uint32_t sh_size;
	uint32_t sh_link;
	uint32_t sh_info;
	uint32_t sh_addralign;
	uint32_t sh_entsize;
};

struct elf_shdr_64
{
	uint32_t sh_name;
	uint32_t sh_type;
	uint64_t sh_flags;
	uint64_t sh_addr;
	uint64_t sh_offset;
	uint64_t sh_size;
	uint32_t sh_link;
	uint32_t sh_info;
	uint64_t sh_addralign;
	uint64_t sh_entsize;
};

/* The following are used with relocations */
#define ELF_32_R_SYM(i) ((i) >> 8)
#define ELF_32_R_TYPE(i) ((unsigned char)(i))
#define ELF_32_R_INFO(s, t) (((s) << 8) + (unsigned char)(t))

#define ELF_64_R_SYM(i) ((i) >> 32)
#define ELF_64_R_TYPE(i) ((i) & 0xffffffffL)
#define ELF_64_R_INFO(s, t) (((s) << 32) + ((t) & 0xffffffffL))

struct elf_rel_32_t
{
	uint32_t r_offset;
	uint32_t r_info;
};

struct elf_rela_32_t
{
	uint32_t r_offset;
	uint32_t r_info;
	int32_t r_addend;
};

struct elf_rel_64_t
{
	uint64_t r_offset;
	uint64_t r_info;
};

struct elf_rela_64_t
{
	uint64_t r_offset;
	uint64_t r_info;
	int64_t r_addend;
};

/* This is the info that is needed to parse the dynamic section of the file */
#define DT_NULL 0
#define DT_NEEDED 1
#define DT_PLTRELSZ 2
#define DT_PLTGOT 3
#define DT_HASH 4
#define DT_STRTAB 5
#define DT_SYMTAB 6
#define DT_RELA 7
#define DT_RELASZ 8
#define DT_RELAENT 9
#define DT_STRSZ 10
#define DT_SYMENT 11
#define DT_INIT 12
#define DT_FINI 13
#define DT_SONAME 14
#define DT_RPATH 15
#define DT_SYMBOLIC 16
#define DT_REL 17
#define DT_RELSZ 18
#define DT_RELENT 19
#define DT_PLTREL 20
#define DT_DEBUG 21
#define DT_TEXTREL 22
#define DT_JMPREL 23
#define DT_ENCODING 32
#define OLD_DT_LOOS 0x60000000
#define DT_LOOS 0x6000000d
#define DT_HIOS 0x6ffff000
#define DT_VALRNGLO 0x6ffffd00
#define DT_VALRNGHI 0x6ffffdff
#define DT_ADDRRNGLO 0x6ffffe00
#define DT_ADDRRNGHI 0x6ffffeff
#define DT_VERSYM 0x6ffffff0
#define DT_RELACOUNT 0x6ffffff9
#define DT_RELCOUNT 0x6ffffffa
#define DT_FLAGS_1 0x6ffffffb
#define DT_VERDEF 0x6ffffffc
#define DT_VERDEFNUM 0x6ffffffd
#define DT_VERNEED 0x6ffffffe
#define DT_VERNEEDNUM 0x6fffffff
#define OLD_DT_HIOS 0x6fffffff
#define DT_LOPROC 0x70000000
#define DT_HIPROC 0x7fffffff

struct elf_dyn_32_t
{
	int32_t d_tag;
	union
	{
		int32_t d_val;
		uint32_t d_ptr;
	} d_un;
};

struct elf_dyn_64_t
{
	int64_t d_tag; /* entry tag value */
	union
	{
		uint64_t d_val;
		uint64_t d_ptr;
	} d_un;
};

static inline int is_elf_hdr(struct elf_hdr_64_t *hdr)
{
	if (hdr->e_ident[ELF_EI_OFFSET_MAG0] == ELF_MAG0 
	&& hdr->e_ident[ELF_EI_OFFSET_MAG1] == ELF_MAG1
	&& hdr->e_ident[ELF_EI_OFFSET_MAG2] == ELF_MAG2
	&& hdr->e_ident[ELF_EI_OFFSET_MAG3] == ELF_MAG3)
		return 1;

	return 0;
}

int validate_elf(struct elf_hdr_64_t* hdr);

int load_elf(vm_t *vm, struct elf_hdr_64_t* hdr);

#endif