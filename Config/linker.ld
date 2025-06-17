ENTRY(start)

SECTIONS
{
  . = 1M;

  .boot :
  {
    KEEP(*(.multiboot_header))
  }

  .text :
  {
    *(.text*)
  }

  .rodata :
  {
    *(.rodata*)
  }

  .data :
  {
    *(.data*)
  }

  .bss :
  {
    __bss_start = .;
    *(.bss*)
    *(COMMON)
    __bss_end = .;
  }
  /DISCARD/ : 
  {
    *(.eh_frame)
    *(.comment)
  }

  . = . + 2M;
  heap_end = .;
}
