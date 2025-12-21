/* FKernel linker script - Multiboot2 friendly */

ENTRY(start)

PHDRS
{
    bootloader PT_LOAD FLAGS(5); /* R_X */
    kernel PT_LOAD FLAGS(5);     /* R_X */
    data   PT_LOAD FLAGS(6);     /* RW_ */
}

/* --- Layout of the binary --- */
SECTIONS
{
    /* --- Multiboot2 Header --- */
    /* Must be located within the first 32 KiB of the final binary */
    . = 0x0;  /* file start offset */
    .multiboot_header : {
        KEEP(*(.multiboot_header))  /* ensure linker does not discard this section */
    } :bootloader

    /* --- Bootloader Sections --- */
    . = ALIGN(0x1000); /* Align to 4K after multiboot header */
    .bootloader.text : {
        *(.prekernel.text*)
    } :bootloader

    .bootloader.rodata : {
        *(.prekernel.rodata*)
    } :bootloader

    .bootloader.data : {
        *(.prekernel.data*)
    } :bootloader

    .bootloader.bss (NOLOAD) : {
        *(.prekernel.bss*)
    } :bootloader

    /* --- Kernel Text --- */
    /* Kernel is loaded at 1MB virtual address (standard x86_64) */
    . = 1M;
    .text : {
        __kernel_start = .;
        *(.text*)
        *(.kernel.text*)
    } :kernel

    .rodata : {
        *(.rodata*)
        *(.kernel.rodata*)
    } :kernel

    /* --- Read-Write Data --- */
    .data : {
        *(.data*)
        *(.kernel.data*)
    } :data

    .bss : {
        *(.bss*)
        *(.kernel.bss*)
        __kernel_end = .;
    } :data

    /* --- Kernel Heap --- */
    .kernel_heap (NOLOAD) : {
        __heap_start = .;
        . += 32M;       /* reserve 32MB for kernel heap */
        __heap_end = .;
    }

    .phys_bitmap (NOLOAD) : ALIGN(4096) {
        __pmm_bitmap_start = .;
        . += 256M;          /* 4 MB bitmap */
        __pmm_bitmap_end = .;
    }
}
