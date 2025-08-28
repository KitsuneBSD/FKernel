section .multiboot_header

%define MULTIBOOT2_MAGIC 0xe85250d6
%define MULTIBOOT2_i386_ARCHICTECTURE 0

header_end:
  dd MULTIBOOT2_MAGIC
  dd MULTIBOOT2_i386_ARCHICTECTURE
  dd header_end - header_start
  dd 0x100000000 - ( MULTIBOOT2_MAGIC + MULTIBOOT2_i386_ARCHICTECTURE + (header_end - header_start))

  dw 0
  dw 0
  dd 8
header_start:
