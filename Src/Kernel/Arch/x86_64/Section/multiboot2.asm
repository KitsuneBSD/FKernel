%define MULTIBOOT2_MAGIC 0xe85250d6
%define MULTIBOOT2_i386_ARCHITECTURE 0

section .multiboot_header
align 8
header_start:
  dd MULTIBOOT2_MAGIC
  dd MULTIBOOT2_i386_ARCHITECTURE
  dd header_end - header_start
  dd -(MULTIBOOT2_MAGIC + MULTIBOOT2_i386_ARCHITECTURE + (header_end - header_start))

  ; Framebuffer Tag (Type 5)
  align 8
  dw 5          ; Type
  dw 0          ; Flags (not optional)
  dd 20         ; Size
  dd 1024       ; Width
  dd 768        ; Height
  dd 32         ; BPP

  ; End tag obrigatório
  align 8
  dw 0          ; Type
  dw 0          ; Flags
  dd 8          ; Size
header_end:
