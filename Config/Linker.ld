ENTRY(_start)

KERNEL_VMA = 0x100000;

SECTIONS {
    . = KERNEL_VMA;

    .multiboot_header : {
        *(.multiboot_header)
    }

    .text : {
        _code = .;
        *(.text)
        *(.rodata*)
        . = ALIGN(4096);
    }

    .data : {
        _data = .;
        *(.data)
        . = ALIGN(4096);
    }

    .eh_frame : {
        _ehframe = .;
        *(.eh_frame)
        . = ALIGN(4096);
    }

    .bss : {
        _bss = .;
        *(.bss)
        *(COMMON)
        . = ALIGN(4096);
    }

    _end = .;

    /DISCARD/ :
    {
        *(.comment)
    }
}

