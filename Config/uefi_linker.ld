/* FKernel UEFI Application Linker Script */
/* PE/COFF format for UEFI applications */

ENTRY(efi_main)

SECTIONS
{
    /* PE header and sections */
    . = 0;
    
    .text : {
        *(.text)
        *(.text*)
        *(.gnu.linkonce.t*)
    }
    
    .rodata : {
        *(.rodata)
        *(.rodata*)
        *(.gnu.linkonce.r*)
    }
    
    .data : {
        *(.data)
        *(.data*)
        *(.gnu.linkonce.d*)
    }
    
    .bss : {
        *(.bss)
        *(.bss*)
        *(.gnu.linkonce.b*)
        *(COMMON)
    }
    
    /* UEFI relocations */
    .reloc : {
        *(.reloc)
    }
    
    /DISCARD/ : {
        *(.comment)
        *(.note*)
        *(.eh_frame)
    }
}