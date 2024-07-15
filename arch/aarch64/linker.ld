ENTRY (_start)

SECTIONS
{
    . = 0x40400000;
    kernelstart = .;

    .boot.text : {
        *(.boot.text*)
    }

    kernelvoffset = 0x70000000000;
    . += kernelvoffset;
    kernelvstart = kernelstart + kernelvoffset;
    kernelvbase = .;

    .text ALIGN(4k) : AT( ADDR (.text) - kernelvoffset )
    { 
        *(.text*) 
        *(VECTORS*) 

        __start_init = ALIGN(8);
        KEEP(*(mod.init*))
        __stop_init = .;

        __start_tests = .;
        KEEP(*(.tests*))
        __stop_tests = .;
    }

    .rodata ALIGN(4k) : AT( ADDR (.rodata) - kernelvoffset )
    { 
        *(.rodata*)
    }

    .data ALIGN(4k) : AT( ADDR (.data) - kernelvoffset )
    { 
        *(EXCLUDE_FILE(*init.o) .data*)
    }

    .initdata ALIGN(4k) : AT( ADDR (.initdata) - kernelvoffset )
    { 
        *(.data*)
    }

    .bss ALIGN(4k) : AT( ADDR (.bss) - kernelvoffset )
    {
        *(COMMON*)
        *(.bss*)

        . = ALIGN(8);
        . += 0x20000; /* 128kB of stack memory */
        stack = .;
        . += 0x1;
        . = ALIGN(8);
    }

    .vdso.text ALIGN(4k): AT( ADDR (.vdso.text) - kernelvoffset )
    {
        vdsostart = .;
        KEEP(*(.vdso.text*))
        vdsoend = .;

        . = ALIGN(4k);
    }

    kernelend = .;

}