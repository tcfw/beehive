#ifndef _KERNEL_ARCH_GIC_H
#define _KERNEL_ARCH_GIC_H

#include "stdint.h"

#define GIC_DIST_BASE 0x08000000
#define GIC_CPU_BASE 0x08010000
#define GIC_REDIST_BASE 0x080A0000

#define GIC_DIST_ISENABLER 0x0100
#define GIC_DIST_ICPENDR 0x0280
#define GIC_DIST_ITARGETSR 0x0800
#define GIC_DIST_ICFGR 0xC00

#define GIC_CPU_PMR 0x0004
#define GIC_CPU_BPR 0x0008

#define GIC_DIST_CONFIG_LEVEL_SENSITIVE 0x0
#define GIC_DIST_CONFIG_EDGE_TRIGGER 0x1

// Set security/group of the specified INTID
// INTID = INTID of interrupt (ID must be less than 32)
// rd    = Redistributor number (ignored if INTID is a SPI)
// group = Security/group setting
#define GICV3_GROUP0 0
#define GICV3_GROUP1_SECURE 1
#define GICV3_GROUP1_NON_SECURE 2

// Set the GIC base address
void setGICAddr(void *dist, void *rdist, void *cpu);

// Get the redistributor id for the affinity
uint32_t getRedistID(uint32_t affinity);

// Enable the redistributor
void gic_redist_enable(uint32_t rd);
// Set the interrupt priority on the redistributor
void gic_redist_set_int_priority(uint32_t xrq, uint32_t rd, uint8_t priority);
// Set the interrupt group for the interrupt on the redistributor
void gic_redist_set_int_group(uint32_t xrq, uint32_t rd, uint32_t security);

// Enable the interrupt number for the redistributor
void gic_redist_enable_int(uint32_t xrq, uint32_t rd);

// Enable the global distributor
void gic_dist_enable();
// Enable the interrupt number on the global distributor
void gic_dist_enable_xrq_n(uint32_t n, uint32_t xrq);

#define GICV3_ROUTE_MODE_ANY (0x80000000)
#define GICV3_ROUTE_MODE_COORDINATE (0)

// Set the interrupt affinity target on the interrupt number
void gic_dist_target(uint32_t xrq, uint32_t mode, uint32_t affinity);

#define GICV3_CONFIG_LEVEL (0)
#define GICV3_CONFIG_EDGE (2)

// Set the interrupt config for on the distributor
void gic_dist_xrq_config(uint32_t xrq, uint32_t type);

// Clear the pending interrupt on the distributor
void gic_dist_clear_n(uint8_t n, uint32_t xrq);

// Init the local CPU interface
void gic_cpu_init();

// Enable the local CPU interface
void gic_cpu_enable();

// Disable the local CPU interface
void gic_cpu_disable();

// Set the priority mask on the CPU interface
void gic_cpu_set_priority_mask(uint8_t mask);

// Mark end of interrupt for group 1 interrupts on the CPU interface
void gic_cpu_eoi_gp1(uint32_t xpr);

struct GICv3_dist_if
{
	volatile uint32_t GICD_CTLR;		// +0x0000 - RW - Distributor Control Register
	const volatile uint32_t GICD_TYPER; // +0x0004 - RO - Interrupt Controller Type Register
	const volatile uint32_t GICD_IIDR;	// +0x0008 - RO - Distributor Implementer Identification Register

	const volatile uint32_t padding0; // +0x000C - RESERVED

	volatile uint32_t GICD_STATUSR; // +0x0010 - RW - Status register

	const volatile uint32_t padding1[3]; // +0x0014 - RESERVED

	volatile uint32_t IMP_DEF[8]; // +0x0020 - RW - Implementation defined registers

	volatile uint32_t GICD_SETSPI_NSR; // +0x0040 - WO - Non-secure Set SPI Pending (Used when SPI is signalled using MSI)
	const volatile uint32_t padding2;  // +0x0044 - RESERVED
	volatile uint32_t GICD_CLRSPI_NSR; // +0x0048 - WO - Non-secure Clear SPI Pending (Used when SPI is signalled using MSI)
	const volatile uint32_t padding3;  // +0x004C - RESERVED
	volatile uint32_t GICD_SETSPI_SR;  // +0x0050 - WO - Secure Set SPI Pending (Used when SPI is signalled using MSI)
	const volatile uint32_t padding4;  // +0x0054 - RESERVED
	volatile uint32_t GICD_CLRSPI_SR;  // +0x0058 - WO - Secure Clear SPI Pending (Used when SPI is signalled using MSI)

	const volatile uint32_t padding5[3]; // +0x005C - RESERVED

	volatile uint32_t GICD_SEIR; // +0x0068 - WO - System Error Interrupt Register (Note: This was recently removed from the spec)

	const volatile uint32_t padding6[5]; // +0x006C - RESERVED

	volatile uint32_t GICD_IGROUPR[32]; // +0x0080 - RW - Interrupt Group Registers (Note: In GICv3, need to look at IGROUPR and IGRPMODR)

	volatile uint32_t GICD_ISENABLER[32]; // +0x0100 - RW - Interrupt Set-Enable Registers
	volatile uint32_t GICD_ICENABLER[32]; // +0x0180 - RW - Interrupt Clear-Enable Registers
	volatile uint32_t GICD_ISPENDR[32];	  // +0x0200 - RW - Interrupt Set-Pending Registers
	volatile uint32_t GICD_ICPENDR[32];	  // +0x0280 - RW - Interrupt Clear-Pending Registers
	volatile uint32_t GICD_ISACTIVER[32]; // +0x0300 - RW - Interrupt Set-Active Register
	volatile uint32_t GICD_ICACTIVER[32]; // +0x0380 - RW - Interrupt Clear-Active Register

	volatile uint8_t GICD_IPRIORITYR[1024]; // +0x0400 - RW - Interrupt Priority Registers
	volatile uint32_t GICD_ITARGETSR[256];	// +0x0800 - RW - Interrupt Processor Targets Registers
	volatile uint32_t GICD_ICFGR[64];		// +0x0C00 - RW - Interrupt Configuration Registers
	volatile uint32_t GICD_GRPMODR[32];		// +0x0D00 - RW - Interrupt Group Modifier (Note: In GICv3, need to look at IGROUPR and IGRPMODR)
	const volatile uint32_t padding7[32];	// +0x0D80 - RESERVED
	volatile uint32_t GICD_NSACR[64];		// +0x0E00 - RW - Non-secure Access Control Register

	volatile uint32_t GICD_SGIR; // +0x0F00 - WO - Software Generated Interrupt Register

	const volatile uint32_t padding8[3]; // +0x0F04 - RESERVED

	volatile uint32_t GICD_CPENDSGIR[4]; // +0x0F10 - RW - Clear pending for SGIs
	volatile uint32_t GICD_SPENDSGIR[4]; // +0x0F20 - RW - Set pending for SGIs

	const volatile uint32_t padding9[52]; // +0x0F30 - RESERVED

	// GICv3.1 extended SPI range
	volatile uint32_t GICD_IGROUPRE[128];	// +0x1000 - RW - Interrupt Group Registers (GICv3.1)
	volatile uint32_t GICD_ISENABLERE[128]; // +0x1200 - RW - Interrupt Set-Enable Registers (GICv3.1)
	volatile uint32_t GICD_ICENABLERE[128]; // +0x1400 - RW - Interrupt Clear-Enable Registers (GICv3.1)
	volatile uint32_t GICD_ISPENDRE[128];	// +0x1600 - RW - Interrupt Set-Pending Registers (GICv3.1)
	volatile uint32_t GICD_ICPENDRE[128];	// +0x1800 - RW - Interrupt Clear-Pending Registers (GICv3.1)
	volatile uint32_t GICD_ISACTIVERE[128]; // +0x1A00 - RW - Interrupt Set-Active Register (GICv3.1)
	volatile uint32_t GICD_ICACTIVERE[128]; // +0x1C00 - RW - Interrupt Clear-Active Register (GICv3.1)

	const volatile uint32_t padding10[128]; // +0x1E00 - RESERVED

	volatile uint8_t GICD_IPRIORITYRE[4096]; // +0x2000 - RW - Interrupt Priority Registers (GICv3.1)

	volatile uint32_t GICD_ICFGRE[256];	   // +0x3000 - RW - Interrupt Configuration Registers (GICv3.1)
	volatile uint32_t GICD_IGRPMODRE[128]; // +0x3400 - RW - Interrupt Group Modifier (GICv3.1)
	volatile uint32_t GICD_NSACRE[256];	   // +0x3600 - RW - Non-secure Access Control Register (GICv3.1)

	const volatile uint32_t padding11[2432]; // +0x3A00 - RESERVED

	// GICv3.0
	volatile uint64_t GICD_ROUTER[1024]; // +0x6000 - RW - Controls SPI routing when ARE=1

	// GICv3.1
	volatile uint64_t GICD_ROUTERE[1024]; // +0x8000 - RW - Controls SPI routing when ARE=1 (GICv3.1)
};

struct GICv3_rdist_lpis_if
{
	volatile uint32_t GICR_CTLR;		   // +0x0000 - RW - Redistributor Control Register
	const volatile uint32_t GICR_IIDR;	   // +0x0004 - RO - Redistributor Implementer Identification Register
	const volatile uint32_t GICR_TYPER[2]; // +0x0008 - RO - Redistributor Type Register
	volatile uint32_t GICR_STATUSR;		   // +0x0010 - RW - Redistributor Status register
	volatile uint32_t GICR_WAKER;		   // +0x0014 - RW - Wake Request Registers
	const volatile uint32_t GICR_MPAMIDR;  // +0x0018 - RO - Reports maximum PARTID and PMG (GICv3.1)
	volatile uint32_t GICR_PARTID;		   // +0x001C - RW - Set PARTID and PMG used for Redistributor memory accesses (GICv3.1)
	const volatile uint32_t padding1[8];   // +0x0020 - RESERVED
	volatile uint64_t GICR_SETLPIR;		   // +0x0040 - WO - Set LPI pending (Note: IMP DEF if ITS present)
	volatile uint64_t GICR_CLRLPIR;		   // +0x0048 - WO - Set LPI pending (Note: IMP DEF if ITS present)
	const volatile uint32_t padding2[6];   // +0x0058 - RESERVED
	volatile uint32_t GICR_SEIR;		   // +0x0068 - WO - (Note: This was removed from the spec)
	const volatile uint32_t padding3;	   // +0x006C - RESERVED
	volatile uint64_t GICR_PROPBASER;	   // +0x0070 - RW - Sets location of the LPI configuration table
	volatile uint64_t GICR_PENDBASER;	   // +0x0078 - RW - Sets location of the LPI pending table
	const volatile uint32_t padding4[8];   // +0x0080 - RESERVED
	volatile uint64_t GICR_INVLPIR;		   // +0x00A0 - WO - Invalidates cached LPI config (Note: In GICv3.x: IMP DEF if ITS present)
	const volatile uint32_t padding5[2];   // +0x00A8 - RESERVED
	volatile uint64_t GICR_INVALLR;		   // +0x00B0 - WO - Invalidates cached LPI config (Note: In GICv3.x: IMP DEF if ITS present)
	const volatile uint32_t padding6[2];   // +0x00B8 - RESERVED
	volatile uint64_t GICR_SYNCR;		   // +0x00C0 - WO - Redistributor Sync
	const volatile uint32_t padding7[2];   // +0x00C8 - RESERVED
	const volatile uint32_t padding8[12];  // +0x00D0 - RESERVED
	volatile uint64_t GICR_MOVLPIR;		   // +0x0100 - WO - IMP DEF
	const volatile uint32_t padding9[2];   // +0x0108 - RESERVED
	volatile uint64_t GICR_MOVALLR;		   // +0x0110 - WO - IMP DEF
	const volatile uint32_t padding10[2];  // +0x0118 - RESERVED
};

struct GICv3_rdist_sgis_if
{
	const volatile uint32_t padding1[32];  // +0x0000 - RESERVED
	volatile uint32_t GICR_IGROUPR[3];	   // +0x0080 - RW - Interrupt Group Registers (Security Registers in GICv1)
	const volatile uint32_t padding2[29];  // +0x008C - RESERVED
	volatile uint32_t GICR_ISENABLER[3];   // +0x0100 - RW - Interrupt Set-Enable Registers
	const volatile uint32_t padding3[29];  // +0x010C - RESERVED
	volatile uint32_t GICR_ICENABLER[3];   // +0x0180 - RW - Interrupt Clear-Enable Registers
	const volatile uint32_t padding4[29];  // +0x018C - RESERVED
	volatile uint32_t GICR_ISPENDR[3];	   // +0x0200 - RW - Interrupt Set-Pending Registers
	const volatile uint32_t padding5[29];  // +0x020C - RESERVED
	volatile uint32_t GICR_ICPENDR[3];	   // +0x0280 - RW - Interrupt Clear-Pending Registers
	const volatile uint32_t padding6[29];  // +0x028C - RESERVED
	volatile uint32_t GICR_ISACTIVER[3];   // +0x0300 - RW - Interrupt Set-Active Register
	const volatile uint32_t padding7[29];  // +0x030C - RESERVED
	volatile uint32_t GICR_ICACTIVER[3];   // +0x0380 - RW - Interrupt Clear-Active Register
	const volatile uint32_t padding8[29];  // +0x018C - RESERVED
	volatile uint8_t GICR_IPRIORITYR[96];  // +0x0400 - RW - Interrupt Priority Registers
	const volatile uint32_t padding9[488]; // +0x0460 - RESERVED
	volatile uint32_t GICR_ICFGR[6];	   // +0x0C00 - RW - Interrupt Configuration Registers
	const volatile uint32_t padding10[58]; // +0x0C18 - RESERVED
	volatile uint32_t GICR_IGRPMODR[3];	   // +0x0D00 - RW - Interrupt Group Modifier Register
	const volatile uint32_t padding11[61]; // +0x0D0C - RESERVED
	volatile uint32_t GICR_NSACR;		   // +0x0E00 - RW - Non-secure Access Control Register
};

struct GICv3_rdist_vlpis_if
{
	const volatile uint32_t padding1[28]; // +0x0000 - RESERVED
	volatile uint64_t GICR_VPROPBASER;	  // +0x0070 - RW - Sets location of the LPI vPE Configuration table
	volatile uint64_t GICR_VPENDBASER;	  // +0x0078 - RW - Sets location of the LPI Pending table
};

struct GICv3_rdist_res_if
{
	const volatile uint32_t padding1[32]; // +0x0000 - RESERVED
};

struct GICv3_rdist_if
{
	struct GICv3_rdist_lpis_if lpis __attribute__((aligned(0x10000)));
	struct GICv3_rdist_sgis_if sgis __attribute__((aligned(0x10000)));

#ifdef GICV4
	struct GICv3_rdist_vlpis_if vlpis __attribute__((aligned(0x10000)));
	struct GICv3_rdist_res_if res __attribute__((aligned(0x10000)));
#endif
};

// +0 from ITS_BASE
struct GICv3_its_ctlr_if
{
	volatile uint32_t GITS_CTLR;		  // +0x0000 - RW - ITS Control Register
	const volatile uint32_t GITS_IIDR;	  // +0x0004 - RO - Implementer Identification Register
	const volatile uint64_t GITS_TYPER;	  // +0x0008 - RO - ITS Type register
	const volatile uint32_t GITS_MPAMIDR; // +0x0010 - RO - Reports maxmimum PARTID and PMG (GICv3.1)
	volatile uint32_t GITS_PARTIDR;		  // +0x0004 - RW - Sets the PARTID and PMG used for ITS memory accesses (GICv3.1)
	const volatile uint32_t GITS_MPIDR;	  // +0x0018 - RO - ITS affinity, used for shared vPE table
	const volatile uint32_t padding5;	  // +0x001C - RESERVED
	volatile uint32_t GITS_IMPDEF[8];	  // +0x0020 - RW - IMP DEF registers
	const volatile uint32_t padding2[16]; // +0x0040 - RESERVED
	volatile uint64_t GITS_CBASER;		  // +0x0080 - RW - Sets base address of ITS command queue
	volatile uint64_t GITS_CWRITER;		  // +0x0088 - RW - Points to next enrty to add command
	volatile uint64_t GITS_CREADR;		  // +0x0090 - RW - Points to command being processed
	const volatile uint32_t padding3[2];  // +0x0098 - RESERVED
	const volatile uint32_t padding4[24]; // +0x00A0 - RESERVED
	volatile uint64_t GITS_BASER[8];	  // +0x0100 - RW - Sets base address of Device and Collection tables
};

// +0x010000 from ITS_BASE
struct GICv3_its_int_if
{
	const volatile uint32_t padding1[16]; // +0x0000 - RESERVED
	volatile uint32_t GITS_TRANSLATER;	  // +0x0040 - RW - Written by peripherals to generate LPI
};

// +0x020000 from ITS_BASE
struct GICv3_its_sgi_if
{
	const volatile uint32_t padding1[8]; // +0x0000 - RESERVED
	volatile uint64_t GITS_SGIR;		 // +0x0020 - RW - Written by peripherals to generate vSGI (GICv4.1)
};

struct GICv2_cpu_if
{
	volatile uint32_t GICC_CTRL;			// 0x0000 - RW - CPU Interface Control Register
	volatile uint32_t GICC_PMR;				// 0x0004 - RW - Interrupt Priority Mask Register
	volatile uint32_t GICC_BPR;				// 0x0008 - RW - Binary Point Register
	const volatile uint32_t GICC_IAR;		// 0x000C - RO - Interrupt Acknowledge Register
	volatile uint32_t GICC_EOIR;			// 0x0010 - WO - End of Interrupt Register
	const volatile uint32_t GICC_RPR;		// 0x0014 - RO - Running Priority Register
	const volatile uint32_t GICC_HPPIR;		// 0x0018 - RO - Highest Priority Pending Interrupt Register
	volatile uint32_t GICC_ABPR;			// 0x001C - RW - Aliased Binary Point Register
	const volatile uint32_t GICC_AIAR;		// 0x0020 - RO - Aliased Interrupt Acknowledge Register
	volatile uint32_t GICC_AEOIR;			// 0x0024 - WO - Aliased End of Interrupt Register
	const volatile uint32_t GICC_AHPPIR;	// 0x0028 - RO - Aliased Highest Priority Pending Interrupt Register
	const volatile uint32_t _reserved1[4];	// 0x002C-0x003C Reserved
	const volatile uint32_t _reserved2[35]; // 0x0040-0x00CF Reserved - implementation defined registers
	volatile uint32_t GICC_APRn;			// 0x00D0-0x00DC - RW - Active Priorities Registers
	volatile uint32_t GICC_NSAPRn;			// 0x00E0-0x00EC - RW - Non-secure Active Priorities Registers
	const volatile uint32_t _reserved3[4];	// 0x00ED-0x00F8 Reserved
	const volatile uint32_t GICC_IIDR;		// 0x00FC - RO - CPU Interface Identification Register
	volatile uint32_t GICC_DIR;				// 0x1000 - WO - Deactivate Interrupt Register
};

#endif