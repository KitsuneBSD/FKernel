%define MULTIBOOT2_MAGIC 0xe85250d6
%define MULTIBOOT2_i386_ARCHITECTURE 0

section .multiboot_header
align 8
header_start:
  dd MULTIBOOT2_MAGIC
  dd MULTIBOOT2_i386_ARCHITECTURE
  dd header_end - header_start
  dd -(MULTIBOOT2_MAGIC + MULTIBOOT2_i386_ARCHITECTURE + (header_end - header_start))
  ; End tag obrigatório
  dw 0
  dw 0
  dd 8
header_end:
