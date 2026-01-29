/* FKernel UEFI Application Linker Script */

ENTRY(efi_main)

SECTIONS
{
    /* UEFI applications typically start at address 0 and are relocatable */
    . = 0;
    __kernel_start = .;
    
    .text : {
        *(.text)
        *(.text*)
        *(.ltext)
        *(.ltext*)
        *(.gnu.linkonce.t*)
    }
    
    . = ALIGN(4096);
    .rodata : {
        *(.rodata)
        *(.rodata*)
        *(.gnu.linkonce.r*)
    }
    
    . = ALIGN(4096);
    .data : {
        *(.data)
        *(.data*)
        *(.ldata)
        *(.ldata*)
        *(.gnu.linkonce.d*)
    }
    
    . = ALIGN(4096);
    .bss : {
        *(.bss)
        *(.bss*)
        *(.lbss)
        *(.lbss*)
        *(COMMON)
    }
    __kernel_end = .;

    /* Kernel heap for UEFI (32MB at 64MB like main kernel) */
    . = ALIGN(4096);
    . = MAX(., 64M);
    .kernel_heap (NOLOAD) : {
        __heap_start = .;
        . += 32M;       /* reserve 32MB for kernel heap */
        __heap_end = .;
    }
    
    /* Physical memory bitmap */
    . = ALIGN(4096);
    .phys_bitmap (NOLOAD) : {
        __pmm_bitmap_start = .;
        . += 32M;          /* 32 MB bitmap supports up to 1TB of RAM */
        __pmm_bitmap_end = .;
    }

    . = ALIGN(4096);
    .rela : {
        *(.rela*)
    }
    
    /DISCARD/ : {
        *(.comment)
        *(.note*)
        *(.eh_frame)
        *(.eh_frame_hdr)
    }
}