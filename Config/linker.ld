ENTRY(start)

SECTIONS {
    . = 1M;

    .multiboot_header ALIGN(8) : {
        KEEP(*(.multiboot_header))
    }

    .kernel : {
        __kernel_start = .;
        *(.text)
        *(.rodata)
        *(.data)
        *(.bss)
        __kernel_end = .;
    }

    .kernel_heap (NOLOAD) : {
        __heap_start = .;
        . += 32M;
        __heap_end = .;
    }
}

