ENTRY (_start)

SECTIONS
{
    . = 0x40000000;

    kernelstart = .;
    kernelvmbase = 0x40000000;

    . = kernelvmbase;
    /* . = 0x7F000000000; */
    /* High mem kernel, otherwise would be limited to 
        1GB user space, but needs a bootloader to set up 
        virtual memory first */


    .text ALIGN(4K) : /* AT(. - kernelvmbase) */
    { 
        KEEP(*(.boot*))

        *(.text*) 

        __start_init = .;
        KEEP(*(.init*))
        __stop_init = .;

        __start_tests = .;
        KEEP(*(.tests*))
        __stop_tests = .;
    }

    .rodata ALIGN(4K) : /* AT(. - kernelvmbase) */
    {
        *(.rodata*)
    }

    .data ALIGN(4K) : /* AT(. - kernelvmbase) */
    { 
        *(COMMON*)
        *(.data*)
        *(.bss*)
    }

    . = ALIGN(64);
    . += 0x20000; /* 128kB of stack memory */
    stack = .;

    . += 0x1;
    . = ALIGN(8);

    kernelend = .;
}