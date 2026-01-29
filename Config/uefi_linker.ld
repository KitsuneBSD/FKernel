/* FKernel UEFI Linker Script for Binary Patching */

ENTRY(efi_main)

SECTIONS
{
    /* We align the code to start at a clean offset for the binary blob */
    . = 0x1000;
    
    .text : {
        *(.text)
        *(.text*)
        *(.ltext)
        *(.ltext*)
    }
    
    .rodata : {
        *(.rodata)
        *(.rodata*)
    }
    
    .data : {
        *(.data)
        *(.data*)
        *(.ldata)
        *(.ldata*)
    }
    
    .bss : {
        *(.bss)
        *(.bss*)
        *(.lbss)
        *(.lbss*)
        *(COMMON)
    }

    /* Keep heap and bitmap far enough */
    . = ALIGN(4096);
    __kernel_end = .;
    
    . = 64M;
    __heap_start = .;
    . += 32M;
    __heap_end = .;
    
    . = ALIGN(4096);
    __pmm_bitmap_start = .;
    . += 32M;
    __pmm_bitmap_end = .;

    /DISCARD/ : {
        *(.comment)
        *(.note*)
        *(.eh_frame)
        *(.rel*)
    }
}