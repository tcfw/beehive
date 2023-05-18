ENTRY (_start)

SECTIONS
{
    . = 0x40000000;
    /* . = 0xffff000000000000; High mem kernel, otherwise would be limited to 1GB user space, but needs a bootloader to set up virtual memory first */

    kernelstart = .;

    .boot : { KEEP(*(.boot*)) }
    .text : { *(.text*) *(.rodata*) }
    .data : { *(.data*) }
    .bss : { *(.bss*) *(COMMON*) }
    . = ALIGN(8);
    . += 0x10000; /* 64kB of stack memory */
    stack = .;
    . += 0x10000; /* 64kB of stack memory */
    intstack = .;

    . += 0x1;
    . = ALIGN(8);
    kernelend = .;
}