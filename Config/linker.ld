ENTRY(start)

PHDRS
{
    headers PT_LOAD FLAGS(5); /* R_X */
    kernel PT_LOAD FLAGS(5); /* R_X */
    data PT_LOAD FLAGS(6); /* RW_ */
}

SECTIONS {
    .multiboot_header ALIGN(8) : {
      KEEP(*(.multiboot_header))
    } :headers

    . = 1M;

    .text : {
        __kernel_start = .;
        *(.text)
    } :kernel

    .rodata : {
        *(.rodata)
    } :kernel

    .data : {
        *(.data)
    } :data

    .bss : {
        *(.bss)
        __kernel_end = .;
    } :data

    .kernel_heap (NOLOAD) : {
        __heap_start = .;
        . += 32M;
        __heap_end = .;
    }
}

