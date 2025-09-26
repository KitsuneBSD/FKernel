ENTRY(start)

SECTIONS
{
    . = 1M;

    .kernel : 
    {
        KEEP(*(.multiboot_header))

        __kernel_start =.;
        *(.text)
        *(.rodata)
        *(.data)
        *(.bss)

        __kernel_end = .;
    }

    .kernel_heap (NOLOAD) :
    {
        __heap_start = .;
        . += 32M;
        __heap_end = .;
    }
}
