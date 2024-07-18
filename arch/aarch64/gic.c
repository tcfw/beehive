#include "gic.h"
#include <kernel/stdint.h>
#include "pe.h"

struct GICv3_dist_if *gic_dist;
struct GICv3_rdist_if *gic_rdist;
struct GICv2_cpu_if *gic_cpu;
static uint32_t gic_max_rd = 0;

// Sets the address of the Distributor and Redistributors
void setGICAddr(void *dist, void *rdist, void *cpu)
{
	uint32_t index = 0;

	gic_dist = (struct GICv3_dist_if *)dist;
	gic_rdist = (struct GICv3_rdist_if *)rdist;
	gic_cpu = (struct GICv2_cpu_if *)cpu;

	// Now find the maximum RD ID that I can use
	// This is used for range checking in later functions
	while ((gic_rdist[index].lpis.GICR_TYPER[0] & (1 << 4)) == 0) // Keep incrementing until GICR_TYPER.Last reports no more RDs in block
	{
		index++;
	}

	gic_max_rd = index;
}

// Get the redistributor for the specified affinity
uint32_t getRedistID(uint32_t affinity)
{
	uint32_t index = 0;

	do
	{
		if (gic_rdist[index].lpis.GICR_TYPER[1] == affinity)
			return index;

		index++;
	} while (index <= gic_max_rd);

	return 0xFFFFFFFF; // return -1 to signal not RD found
}

// enable the redistributor for the given redistributor ID
void gic_redist_enable(uint32_t rd)
{
	uint32_t tmp;

	// Tell the Redistributor to wake-up by clearing ProcessorSleep bit
	tmp = gic_rdist[rd].lpis.GICR_WAKER;
	tmp = tmp & ~0x2;
	gic_rdist[rd].lpis.GICR_WAKER = tmp;

	// Poll ChildrenAsleep bit until Redistributor wakes
	do
	{
		tmp = gic_rdist[rd].lpis.GICR_WAKER;
	} while ((tmp & 0x4) != 0);
}

// Set the redistribtor INTID priority
void gic_redist_set_int_priority(uint32_t xrq, uint32_t rd, uint8_t priority)
{
	if (xrq < 31)
	{
		// SGI or PPI
		gic_rdist[rd].sgis.GICR_IPRIORITYR[xrq] = priority;
	}
	else if (xrq < 1020)
	{
		// SPI
		gic_dist->GICD_IPRIORITYR[xrq] = priority;
	}
	else if ((xrq > 1055) && (xrq < 1120))
	{
		// Extended PPI

		xrq = xrq - 1024;
		gic_rdist[rd].sgis.GICR_IPRIORITYR[xrq] = priority;
	}
	else if ((xrq > 4095) && (xrq < 5120))
	{
		// Extended SPI

		gic_dist->GICD_IPRIORITYRE[(xrq - 4096)] = priority;
	}
}

uint8_t gic_redist_get_int_priority(uint32_t xrq, uint32_t rd)
{
	if (xrq < 31)
	{
		// SGI or PPI
		return gic_rdist[rd].sgis.GICR_IPRIORITYR[xrq];
	}
	else if (xrq < 1020)
	{
		// SPI
		return gic_dist->GICD_IPRIORITYR[xrq];
	}
	else if ((xrq > 1055) && (xrq < 1120))
	{
		// Extended PPI

		xrq = xrq - 1024;
		return gic_rdist[rd].sgis.GICR_IPRIORITYR[xrq];
	}
	else if ((xrq > 4095) && (xrq < 5120))
	{
		// Extended SPI

		return gic_dist->GICD_IPRIORITYRE[(xrq - 4096)];
	}
}

// Set the redistributor INTID group
void gic_redist_set_int_group(uint32_t xrq, uint32_t rd, uint32_t security)
{
	uint32_t bank, group, mod;

	if (xrq < 31)
	{
		// SGI or PPI
		xrq = xrq & 0x1f; // Find which bit within the register
		xrq = 1 << xrq;	  // Move a '1' into the correct bit position

		// Read current values
		group = gic_rdist[rd].sgis.GICR_IGROUPR[0];
		mod = gic_rdist[rd].sgis.GICR_IGRPMODR[0];

		// Update required bits
		switch (security)
		{
		case GICV3_GROUP0:
			group = (group & ~xrq);
			mod = (mod & ~xrq);
			break;

		case GICV3_GROUP1_SECURE:
			group = (group & ~xrq);
			mod = (mod | xrq);
			break;

		case GICV3_GROUP1_NON_SECURE:
			group = (group | xrq);
			mod = (mod & ~xrq);
			break;

		default:
			return;
		}

		// Write modified version back
		gic_rdist[rd].sgis.GICR_IGROUPR[0] = group;
		gic_rdist[rd].sgis.GICR_IGRPMODR[0] = mod;
	}
	else if (xrq < 1020)
	{
		// SPI
		bank = xrq / 32;  // There are 32 IDs per register, need to work out which register to access
		xrq = xrq & 0x1f; // ... and which bit within the register

		xrq = 1 << xrq; // Move a '1' into the correct bit position

		group = gic_dist->GICD_IGROUPR[bank];
		mod = gic_dist->GICD_GRPMODR[bank];

		switch (security)
		{
		case GICV3_GROUP0:
			group = (group & ~xrq);
			mod = (mod & ~xrq);
			break;

		case GICV3_GROUP1_SECURE:
			group = (group & ~xrq);
			mod = (mod | xrq);
			break;

		case GICV3_GROUP1_NON_SECURE:
			group = (group | xrq);
			mod = (mod & ~xrq);
			break;

		default:
			return;
		}

		gic_dist->GICD_IGROUPR[bank] = group;
		gic_dist->GICD_GRPMODR[bank] = mod;
	}
}

// Enable the INTID for the given redistributor
void gic_redist_enable_int(uint32_t xrq, uint32_t rd)
{
	uint32_t bank;

	if (xrq < 31)
	{
		// SGI or PPI
		xrq = xrq & 0x1f; // ... and which bit within the register
		xrq = 1 << xrq;	  // Move a '1' into the correct bit position

		gic_rdist[rd].sgis.GICR_ISENABLER[0] = xrq;
	}
	else if (xrq < 1020)
	{
		// SPI
		bank = xrq / 32;  // There are 32 IDs per register, need to work out which register to access
		xrq = xrq & 0x1f; // ... and which bit within the register

		xrq = 1 << xrq; // Move a '1' into the correct bit position

		gic_dist->GICD_ISENABLER[bank] = xrq;
	}
	else if ((xrq > 1055) && (xrq < 1120))
	{
		xrq = xrq - 1024;
		bank = xrq / 32;  // There are 32 IDs per register, need to work out which register to access
		xrq = xrq & 0x1F; // ... and which bit within the register
		xrq = 1 << xrq;	  // Move a '1' into the correct bit position

		gic_rdist[rd].sgis.GICR_ISENABLER[bank] = xrq;
	}
	else if ((xrq > 4095) && (xrq < 5120))
	{
		// Extended SPI

		xrq = xrq - 4096;
		bank = xrq / 32;  // There are 32 IDs per register, need to work out which register to access
		xrq = xrq & 0x1F; // ... and which bit within the register
		xrq = 1 << xrq;	  // Move a '1' into the correct bit position

		gic_dist->GICD_ISENABLERE[bank] = xrq;
	}
}

// Enable the distributor
void gic_dist_enable()
{
	// Enable ARE for secure & non-secure
	gic_dist->GICD_CTLR = (1 << 5) | (1 << 4);

	// Enable Group 1 S+NS & Group 0
	gic_dist->GICD_CTLR = 0b111 | (1 << 5) | (1 << 4) | (1 << 7);
}

// Enable the INTID in the distributor
void gic_dist_enable_xrq_n(uint32_t n, uint32_t xrq)
{
	xrq = xrq & 0x1f; // ... and which bit within the register
	xrq = 1 << xrq;	  // Move a '1' into the correct bit position
	gic_rdist[n].sgis.GICR_ISENABLER[0] = xrq;
}

// Sets the target CPUs of the specified INTID
void gic_dist_target(uint32_t xrq, uint32_t mode, uint32_t affinity)
{
	uint32_t tmp = (uint64_t)(affinity & 0x00FFFFFF) |
				   (((uint64_t)affinity & 0xFF000000) << 8) |
				   (uint64_t)mode;

	if (!((xrq > 31) && (xrq < 1020)))
	{
		return;
	}

	if ((xrq > 31) && (xrq < 1020))
		gic_dist->GICD_ROUTER[xrq] = tmp;
	else
		gic_dist->GICD_ROUTERE[(xrq - 4096)] = tmp;
}

// Configures the INTID as edge or level sensitive
void gic_dist_xrq_config(uint32_t xrq, uint32_t type)
{
	uint32_t bank, tmp, conf = 0;

	if (xrq < 31)
	{
		// SGI or PPI
		// Config of SGIs is fixed
		// It is IMP DEF whether ICFG for PPIs is write-able, on Arm implementations it is fixed
		return;
	}
	else if (xrq < 1020)
	{
		// SPI
		type = type & 0x3; // Mask out unused bits

		bank = xrq / 16; // There are 16 xrqs per register, need to work out which register to access
		xrq = xrq & 0xF; // ... and which field within the register
		xrq = xrq * 2;	 // Convert from which field to a bit offset (2-bits per field)

		conf = conf << xrq; // Move configuration value into correct bit position

		tmp = gic_dist->GICD_ICFGR[bank]; // Read current value
		tmp = tmp & ~(0x3 << xrq);		  // Clear the bits for the specified field
		tmp = tmp | conf;				  // OR in new configuration
		gic_dist->GICD_ICFGR[bank] = tmp; // Write updated value back
	}
	else if ((xrq > 4095) && (xrq < 5120))
	{
		// Extended SPI

		type = type & 0x3; // Mask out unused bits

		xrq = xrq - 4096;
		bank = xrq / 16; // There are 16 xrqs per register, need to work out which register to access
		xrq = xrq & 0xF; // ... and which field within the register
		xrq = xrq * 2;	 // Convert from which field to a bit offset (2-bits per field)

		conf = conf << xrq; // Move configuration value into correct bit position

		tmp = gic_dist->GICD_ICFGRE[bank]; // Read current value
		tmp = tmp & ~(0x3 << xrq);		   // Clear the bits for the specified field
		tmp = tmp | conf;				   // OR in new configuration
		gic_dist->GICD_ICFGRE[bank] = tmp; // Write updated value back
	}
}

// Init the CPU interface
void gic_cpu_init()
{
	uint64_t ctlr = 0;
	// Legacy: enable system registers
	__asm__ volatile("MOV x0, #1");
	__asm__ volatile("MSR S3_0_C12_C12_5, x0"); // ICC_SRE_EL1

	// Set up EOI mode
	__asm__ volatile("MRS %0, S3_0_c12_c12_4" ::"r"(ctlr)); // ICC_CTLR_EL1
	ctlr |= (1 << 1);
	__asm__ volatile("MSR S3_0_c12_c12_4, %0"
					 : "=r"(ctlr)); // ICC_CTLR_EL1

	__asm__ volatile("ISB");

	// Enable interrupts
	__asm__ volatile("MSR DAIFClr, 0x3");
	__asm__ volatile("ISB");
	__asm__ volatile("MRS x0, DAIF");
	__asm__ volatile("ORR x0, x0, #((1<<7)|(1<<6))");
}

// Enable the CPU interface Group 0 and Group 1 interrupts
void gic_cpu_enable()
{
	// gic_cpu->GICC_CTRL = 1;

	// set binary point registers
	__asm__ volatile("MOV x0, #0");
	__asm__ volatile("MSR S3_0_c12_c8_3, x0");	// ICC_BPR0_EL1
	__asm__ volatile("MSR S3_0_c12_c12_3, x0"); // ICC_BPR1_EL1

	// Enable Group 0
	__asm__ volatile("MOV x0, #1");
	__asm__ volatile("MSR S3_0_C12_C12_6, x0 "); // ICC_IGRPEN0_EL1
	__asm__ volatile("ISB");

	// Enable Group 1
	__asm__ volatile("MRS x0, S3_0_C12_C12_7"); // ICC_IGRPEN1_EL1
	__asm__ volatile("ORR x0, x0, #1");
	__asm__ volatile("MSR S3_0_C12_C12_7, x0"); // ICC_IGRPEN1_EL1
	__asm__ volatile("ISB");

	if (currentEL() == 3)
	{
		// Enable NS Group 1
		__asm__ volatile("MRS x0, S3_6_C12_C12_7"); // ICC_IGRPEN1_EL3
		__asm__ volatile("ORR x0, x0, #1");
		__asm__ volatile("MSR S3_6_C12_C12_7, x0"); // ICC_IGRPEN1_EL3
		__asm__ volatile("ISB");
	}

	// enable interrupts
	__asm__ volatile("MRS x0, S3_0_c12_c12_4"); // ICC_CTRL_EL1
	__asm__ volatile("ORR x0, x0, #1");
	__asm__ volatile("MSR S3_0_c12_c12_4, x0"); // ICC_CTRL_EL1
}

// Disable the CPU interface Group 0 and Group 1 interrupts
void gic_cpu_disable()
{
	// gic_cpu->GICC_CTRL = 0;
	// Disable Group 0
	__asm__ volatile("MOV w0, #0");
	__asm__ volatile("MSR S3_0_C12_C12_6, x0"); // ICC_IGRPEN0_EL1
	__asm__ volatile("ISB");

	// Disable Group 1
	__asm__ volatile("MOV w1, #1");
	__asm__ volatile("MRS x0, S3_0_C12_C12_7"); // ICC_IGRPEN1_EL1
	__asm__ volatile("BIC x0, x0, x1");
	__asm__ volatile("MSR S3_0_C12_C12_7, x0"); // ICC_IGRPEN1_EL1
	__asm__ volatile("ISB");

	if (currentEL() == 3)
	{
		// Disable NS Group 1
		__asm__ volatile("MOV w1, #0x1");
		__asm__ volatile("MRS x0, S3_6_C12_C12_7"); // ICC_IGRPEN1_EL3
		__asm__ volatile("BIC x0, x0, x1");
		__asm__ volatile("MSR S3_6_C12_C12_7, x0"); // ICC_IGRPEN1_EL3
		__asm__ volatile("ISB");
	}

	// TODO(tcfw) actually disable in ICC_CTRL_EL1
}

// Update the CPU interface priority mask
void gic_cpu_set_priority_mask(uint8_t mask)
{
	mask = mask & 0xff;
	__asm__ volatile("MSR S3_0_C4_C6_0, %0" // ICC_PMR_EL1
					 ::"r"(mask));
}

// ACK INTID End of interrupt for group 1 interrupts
void gic_cpu_eoi_gp1(uint32_t xpr)
{
	__asm__ volatile("MSR S3_0_C12_C12_1, %0" // ICC_EOIR1_EL1
					 ::"r"(xpr));
	__asm__ volatile("ISB");
}