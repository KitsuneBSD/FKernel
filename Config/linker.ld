ENTRY(start)

SECTIONS {
    .multiboot_header ALIGN(8) : {
      KEEP(*(.multiboot_header))
    }
  
    . = 1M;


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

