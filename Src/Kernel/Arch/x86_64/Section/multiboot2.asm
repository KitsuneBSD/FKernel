%define MULTIBOOT2_MAGIC 0xe85250d6
%define MULTIBOOT2_i386_ARCHITECTURE 0

section .multiboot_header
align 8
header_start:
  dd MULTIBOOT2_MAGIC
  dd MULTIBOOT2_i386_ARCHITECTURE
  dd header_end - header_start
  dd -(MULTIBOOT2_MAGIC + MULTIBOOT2_i386_ARCHITECTURE + (header_end - header_start))

  ; Console Flags Tag (Type 4)
  align 8
  dw 4          ; Type
  dw 0          ; Flags
  dd 12         ; Size
  dd 3          ; Console required + EGA text supported

  ; Framebuffer Tag (Type 5) - Auto-detect best resolution
  align 8
  dw 5          ; Type
  dw 0          ; Flags (not optional)
  dd 20         ; Size
  dd 0          ; Width (0 = auto-detect best resolution)
  dd 0          ; Height (0 = auto-detect best resolution)
  dd 32         ; BPP

  ; End tag obrigatório
  align 8
  dw 0          ; Type
  dw 0          ; Flags
  dd 8          ; Size
header_end:
