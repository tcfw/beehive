ENTRY (_start)

SECTIONS
{
    . = 0x40000000;
    kernelstart = .;

    .boot.text : {
        *(.boot.text*)
    }

    . += 0x70000000000;
    kernelvstart = . - SIZEOF(.boot.text);
    kernelvbase = .;

    .text ALIGN(4k) : AT( ADDR (.text) - 0x70000000000 )
    { 
        *(.text*) 

        __start_init = ALIGN(8);
        KEEP(*(mod.init*))
        __stop_init = .;

        __start_tests = .;
        KEEP(*(.tests*))
        __stop_tests = .;
    }

    .rodata ALIGN(4k) : AT( ADDR (.rodata) - 0x70000000000 )
    { 
        *(.rodata*)
    }

    .data ALIGN(4k) : AT( ADDR (.data) - 0x70000000000 )
    { 
        *(.data*)
    }

    .bss ALIGN(4k) : AT( ADDR (.bss) - 0x70000000000 )
    {
        *(COMMON*)
        *(.bss*)

        . = ALIGN(8);
        . += 0x20000; /* 128kB of stack memory */
        stack = .;
        . += 0x1;
        . = ALIGN(8);
    }

    kernelend = .;
}