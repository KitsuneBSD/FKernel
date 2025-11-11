ENTRY(start)

PHDRS
{
    kernel PT_LOAD FLAGS(5); /* R_X */
    data   PT_LOAD FLAGS(6); /* RW_ */
}

SECTIONS
{
    . = 1M;

    .text : {
        KEEP(*(.multiboot_header))
        __kernel_start = .;
        *(.text*)
    } :kernel

    .rodata : {
        *(.rodata*)
    } :kernel

    .data : {
        *(.data*)
    } :data

    .bss : {
        *(.bss*)
        __kernel_end = .;
    } :data

    .kernel_heap (NOLOAD) : {
        __heap_start = .;
        . += 32M;
        __heap_end = .;
    }
}
