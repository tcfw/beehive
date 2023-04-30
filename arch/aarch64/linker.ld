ENTRY (_start)

SECTIONS
{
    . = 0x40001000;
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